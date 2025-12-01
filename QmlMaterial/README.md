# Material Design 3 for Qml
This is a library of Qml implementing Google's Material Design.

## 🌐 [Live Demo](https://hypengw.github.io/QmlMaterialWeb/)

### Dependencies

- Qt 6.8+

## [Doc](https://hypengw.github.io/QmlMaterial/index.html)
Work in progress

## Installation

- [Git LFS](https://git-lfs.com) is required to clone fonts.

```cmake
FetchContent_Declare(
  qml_material
  GIT_REPOSITORY https://github.com/hypengw/QmlMaterial.git
  GIT_TAG main
  # GIT_TAG <commit>
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(qml_material)
```

## Usage
```qml
import Qcm.Material as MD

MD.Text {
  text: 'hello world'
}
```
> [!NOTE]
> Never import `Qcm.Material` without `as`, unless you know what you are doing.  

### No QtQuick.Controls
This library only use `QtQuick.Templates`.  
No needs to import `QtQuick.Controls`, unless you need control from Quick Control Style.  

Some type annotations:  
- not accept `Action`, use `MD.Action` instead
- not accept Attached Properties/Signals form `QtQuick.Controls`, use `QtQuick.Templates as T`
  ```qml
    import QtQuick.Templates as T

    T.Overlay.modal: ...
    T.ScrollBar.vertical: ...
    ...
  ```

### Variation Icon
CMake option: `QM_ICON_FONT_URL`  
The default is embedded static round icon with `wght=400 opsz=24 GRAD=0 FILL=0/1`.  
There is also an embedded partial variation one: `qrc:/Qcm/Material/assets/MaterialSymbolsRounded.wght_400.opsz_24.woff2`.  
Set the option to it if you need animation when switching `FILL`.    

> [!NOTE]
> The full variation font will cause huge memory usage. It's better to make partial with a [font tool](https://fonttools.readthedocs.io/en/latest/index.html).    

### Acknowledgements
- [material-color-utilities](https://github.com/material-foundation/material-color-utilities)
- [kirigami](https://invent.kde.org/frameworks/kirigami)
- [quickshell](https://git.outfoxxed.me/quickshell/quickshell)
