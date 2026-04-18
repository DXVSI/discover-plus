#!/usr/bin/env python3

# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 Aleix Pol i Gonzalez <aleixpol@kde.org>

"""
A mock implementation of the sysupdate1 daemon so we can test Discover against
it and test the update process.

Run discover with DISCOVER_TEST_SYSUPDATE=1 so it finds it without running it
as root.
"""

import dbus
import dbus.service
import dbus.mainloop.glib
from gi.repository import GLib
import json
import time
import threading
import uuid
from typing import Dict, Tuple
from pathlib import Path

# D-Bus service and interface names
SERVICE_NAME = "org.freedesktop.sysupdate1"
MANAGER_INTERFACE = "org.freedesktop.sysupdate1.Manager"
TARGET_INTERFACE = "org.freedesktop.sysupdate1.Target"
JOB_INTERFACE = "org.freedesktop.sysupdate1.Job"

FAILURE_WRONGURL = False

class MockJob(dbus.service.Object):
    def __init__(self, bus, path, job_id, manager, job_type, offline=False):
        super().__init__(bus, path)
        self._id = job_id
        self._type = job_type
        self._offline = offline
        self._progress = 0
        self._cancelled = False
        self._path = path
        self._manager = manager

        # Start progress simulation in a separate thread
        self._progress_thread = threading.Thread(target=self._simulate_progress)
        self._progress_thread.daemon = True
        self._progress_thread.start()

    def _simulate_progress(self):
        while self._progress < 100 and not self._cancelled:
            time.sleep(0.1)
            if not self._cancelled:
                self._progress = min(100, self._progress + 5)
                self.PropertiesChanged(
                    JOB_INTERFACE,
                    {"Progress": dbus.UInt32(self._progress)},
                    []
                )
        self._manager.remove_job(self._id)

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature='ss', out_signature='v')
    def Get(self, interface_name, property_name):
        if interface_name == JOB_INTERFACE:
            if property_name == "Id":
                return dbus.UInt64(self._id)
            elif property_name == "Type":
                return dbus.String(self._type)
            elif property_name == "Offline":
                return dbus.Boolean(self._offline)
            elif property_name == "Progress":
                return dbus.UInt32(self._progress)
        raise dbus.exceptions.DBusException(
            'org.freedesktop.DBus.Error.InvalidArgs',
            f'Property {property_name} not found in interface {interface_name}'
        )

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature='s', out_signature='a{sv}')
    def GetAll(self, interface_name):
        if interface_name == JOB_INTERFACE:
            return {
                "Id": dbus.UInt64(self._id),
                "Type": dbus.String(self._type),
                "Offline": dbus.Boolean(self._offline),
                "Progress": dbus.UInt32(self._progress)
            }
        return {}

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature='ssv')
    def Set(self, interface_name, property_name, value):
        raise dbus.exceptions.DBusException(
            'org.freedesktop.DBus.Error.PropertyReadOnly',
            f'Property {property_name} is read-only'
        )

    @dbus.service.method(JOB_INTERFACE)
    def Cancel(self):
        self._cancelled = True
        self._progress = 100
        return

    @dbus.service.signal(dbus.PROPERTIES_IFACE, signature='sa{sv}as')
    def PropertiesChanged(self, interface_name, changed_properties, invalidated_properties):
        pass


class MockTarget(dbus.service.Object):
    def __init__(self, bus, path, name, target_class="os"):
        super().__init__(bus, path)
        self._name = name
        self._class = target_class
        self._path = path
        self._current_version = "1.0.0"
        self._available_versions = ["1.0.0", "1.1.0", "1.2.0", "2.0.0"]
        self._manager = None
        self._bus = bus

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature='ss', out_signature='v')
    def Get(self, interface_name, property_name):
        if interface_name == TARGET_INTERFACE:
            if property_name == "Class":
                return dbus.String(self._class)
            elif property_name == "Name":
                return dbus.String(self._name)
            elif property_name == "Path":
                return dbus.String(self._path)
        raise dbus.exceptions.DBusException(
            'org.freedesktop.DBus.Error.InvalidArgs',
            f'Property {property_name} not found in interface {interface_name}'
        )

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature='s', out_signature='a{sv}')
    def GetAll(self, interface_name):
        if interface_name == TARGET_INTERFACE:
            return {
                "Class": dbus.String(self._class),
                "Name": dbus.String(self._name),
                "Path": dbus.String(self._path)
            }
        return {}

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature='ssv')
    def Set(self, interface_name, property_name, value):
        raise dbus.exceptions.DBusException(
            'org.freedesktop.DBus.Error.PropertyReadOnly',
            f'Property {property_name} is read-only'
        )

    @dbus.service.method(TARGET_INTERFACE, in_signature='b', out_signature='as')
    def List(self, offline):
        return self._available_versions

    @dbus.service.method(TARGET_INTERFACE, in_signature='sb', out_signature='s')
    def Describe(self, version, offline):
        description = f"""{
                "version" : "{version}",
                "newest" : true,
                "available" : true,
                "installed" : true,
                "obsolete" : false,
                "protected" : true,
                "incomplete" : false,
                "changelogUrls" : [],
                "contents" : [
                    {
                        "type" : "regular-file",
                        "path" : "/system/kde-linux_202508240429.erofs",
                        "mtime" : 1756074882866705,
                        "mode" : 416
                    },
                    {
                        "type" : "regular-file",
                        "path" : "/boot/EFI/Linux/kde-linux_202508240429.efi",
                        "mtime" : 1756082194000000,
                        "mode" : 384
                    }
                ]
            }
            """
        return description

    @dbus.service.method(TARGET_INTERFACE, out_signature='s')
    def CheckNew(self):
        latest = self._available_versions[-1]
        if latest > self._current_version:
            return latest
        return ""

    @dbus.service.method(TARGET_INTERFACE, in_signature='st', out_signature='sto')
    def Acquire(self, new_version, flags):
        job_id = int(time.time() * 1000)
        job_path = f"/org/freedesktop/sysupdate1/job/{job_id}"

        if self._manager:
            job = MockJob(self._bus, job_path, job_id, self._manager, "acquire", bool(flags & 1))
            self._manager._jobs[job_id] = (job, job_path)
        return (new_version, dbus.UInt64(job_id), dbus.ObjectPath(job_path))

    @dbus.service.method(TARGET_INTERFACE, in_signature='st', out_signature='sto')
    def Install(self, new_version, flags):
        job_id = int(time.time() * 1000)
        job_path = f"/org/freedesktop/sysupdate1/job/{job_id}"

        if self._manager:
            job = MockJob(self._bus, job_path, job_id, self._manager, "install", bool(flags & 1))
            self._manager._jobs[job_id] = (job, job_path)
        return (new_version, dbus.UInt64(job_id), dbus.ObjectPath(job_path))

    @dbus.service.method(TARGET_INTERFACE, out_signature='u')
    def Vacuum(self):
        # Simulate removing 2 old versions
        return dbus.UInt32(2)

    @dbus.service.method(TARGET_INTERFACE, out_signature='as')
    def GetAppStream(self):
        if FAILURE_WRONGURL:
            return [
                "https://banana.kde.org/maybe.xml"
            ]
        else:
            return [
                "https://invent.kde.org/kde-linux/kde-linux/-/raw/master/org.kde.linux.metainfo.xml"
            ]

    @dbus.service.method(TARGET_INTERFACE, out_signature='s')
    def GetVersion(self):
        return self._current_version

    @dbus.service.signal(dbus.PROPERTIES_IFACE, signature='sa{sv}as')
    def PropertiesChanged(self, interface_name, changed_properties, invalidated_properties):
        pass


class MockSysupdateManager(dbus.service.Object):
    def __init__(self, bus, path="/org/freedesktop/sysupdate1"):
        super().__init__(bus, path)
        self._bus = bus
        self._targets: Dict[str, MockTarget] = {}
        self._jobs: Dict[int, Tuple[MockJob, str]] = {}

        self._create_mock_targets()

    def _create_mock_targets(self):
        targets_data = [
            ("os", "os", "/org/freedesktop/sysupdate1/target/os")
        ]

        for name, target_class, path in targets_data:
            target = MockTarget(self._bus, path, name, target_class)
            target._manager = self
            self._targets[name] = target

    @dbus.service.method(MANAGER_INTERFACE, out_signature='a(sso)')
    def ListTargets(self):
        result = []
        for name, target in self._targets.items():
            result.append((
                target._class,
                target._name,
                dbus.ObjectPath(target._path)
            ))
        return result

    @dbus.service.method(MANAGER_INTERFACE, out_signature='a(tsuo)')
    def ListJobs(self):
        result = []
        for job_id, (job, path) in self._jobs.items():
            result.append((
                dbus.UInt64(job_id),
                job._type,
                dbus.UInt32(job._progress),
                dbus.ObjectPath(path)
            ))
        return result

    @dbus.service.method(MANAGER_INTERFACE, out_signature='as')
    def ListAppStream(self):
        return [
            "https://invent.kde.org/kde-linux/kde-linux/-/raw/master/org.kde.linux.metainfo.xml"
        ]

    @dbus.service.signal(MANAGER_INTERFACE, signature='toi')
    def JobRemoved(self, job_id, path, status):
        pass

    @dbus.service.method(dbus.INTROSPECTABLE_IFACE, out_signature='s')
    def Introspect(self):
        baseDir = Path(__file__).resolve().parent.parent
        return (baseDir / "sysupdate1.xml").read_text(encoding="utf-8")

    @dbus.service.method(dbus.PEER_IFACE)
    def Ping(self):
        pass

    @dbus.service.method(dbus.PEER_IFACE, out_signature='s')
    def GetMachineId(self):
        return str(uuid.uuid4())

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature='ss', out_signature='v')
    def Get(self, interface_name, property_name):
        raise dbus.exceptions.DBusException(
            'org.freedesktop.DBus.Error.InvalidArgs',
            f'Property {property_name} not found in interface {interface_name}'
        )

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature='s', out_signature='a{sv}')
    def GetAll(self, interface_name):
        return {}

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature='ssv')
    def Set(self, interface_name, property_name, value):
        raise dbus.exceptions.DBusException(
            'org.freedesktop.DBus.Error.PropertyReadOnly',
            'Properties are read-only'
        )

    @dbus.service.signal(dbus.PROPERTIES_IFACE, signature='sa{sv}as')
    def PropertiesChanged(self, interface_name, changed_properties, invalidated_properties):
        pass

    def remove_job(self, job_id, status=0):
        if job_id in self._jobs:
            job, path = self._jobs[job_id]
            del self._jobs[job_id]
            self.JobRemoved(dbus.UInt64(job_id), dbus.ObjectPath(path), dbus.Int32(status))


def main():
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = dbus.SessionBus()

    name = dbus.service.BusName(SERVICE_NAME, bus)

    manager = MockSysupdateManager(bus)

    print(f"Mock systemd-sysupdate service started on {SERVICE_NAME}")
    print("Available targets:")
    for target_info in manager.ListTargets():
        print(f"  - {target_info[1]} (class: {target_info[0]}, path: {target_info[2]})")

    mainloop = GLib.MainLoop()
    try:
        mainloop.run()
    except KeyboardInterrupt:
        print("\nShutting down mock service...")
        mainloop.quit()


if __name__ == "__main__":
    main()
