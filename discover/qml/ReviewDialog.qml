import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

Kirigami.Dialog {
    id: reviewDialog

    required property Discover.AbstractResource application
    required property Discover.AbstractReviewsBackend backend

    readonly property alias rating: ratingInput.value
    readonly property alias name: nameInput.text
    readonly property alias summary: titleInput.text
    readonly property alias review: reviewInput.text

    property bool appHasAnyProprietaryLicenses: false

    implicitWidth: Kirigami.Units.gridUnit * 30
    topPadding: 0
    leftPadding: Kirigami.Units.largeSpacing
    rightPadding: Kirigami.Units.largeSpacing
    bottomPadding: Kirigami.Units.largeSpacing

    title: i18n("Reviewing %1", application.name)

    standardButtons: Kirigami.Dialog.NoButton

    customFooterActions: [
        Kirigami.Action {
            id: submitAction
            text: i18n("Submit review")
            icon.name: "document-send"
            enabled: !instructionalLabel.visible
            onTriggered: reviewDialog.accept();
        }
    ]

    contentItem: ColumnLayout {
        Layout.fillWidth: true

        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            Layout.fillWidth: true

            text: i18n("If you love %1, tell people what's great about it! Focus on functionality, usability, and appearance. If the app didn't meet your needs, explain why.", reviewDialog.application.name)
            wrapMode: Text.Wrap
            textFormat: Text.PlainText

            visible: !reviewMightBeCrapWarning.visible
        }

        Kirigami.InlineMessage {
            id: reviewMightBeCrapWarning

            readonly property bool packagedByDistro: ![
                "flatpak-backend",
                "snap-backend",
                "kns-backend",
                "fwupd-backend"
            ].includes(reviewDialog.application.backend.name);
            readonly property string appBugReportUrl: reviewDialog.application.bugURL.toString()

            Layout.fillWidth: true

            type: Kirigami.MessageType.Warning
            text: {
                if (reviewInput.hasDoesntRunKeywords) {
                    if (packagedByDistro) {
                        return xi18nc("@info", "If %1 isn't launching, consider reporting this to its packagers in %2 at <link url='%3'>%3</link> instead, as it may be an easily fixable packaging issue rather than a flaw in the app itself.",
                                    reviewDialog.application.name,
                                    Discover.ResourcesModel.distroName,
                                    Discover.ResourcesModel.distroBugReportUrl());
                    } else {
                        return xi18nc("@info", "If %1 isn't launching, consider reporting this its developers at <link url='%2'>%2</link> instead, as it may be an easily fixable packaging issue rather than a flaw in the app itself.",
                                    reviewDialog.application.name,
                                    appBugReportUrl);
                    }
                } else if (reviewInput.hasCrashKeywords && appBugReportUrl.length > 0){
                    return xi18nc("@info", "If the app is crashing a lot, consider reporting this to the developers at <link url='%1'>%1</link> instead.",
                                appBugReportUrl);
                } else if (reviewInput.hasSubjectiveKeywords && appBugReportUrl.length > 0) {
                    return xi18nc("@info", "If you're having a problem with the app, consider reporting it to the developers at <link url='%1'>%1</link> instead, or else describe the problem here in greater detail.",
                                appBugReportUrl);
                } else {
                    return ""
                }
            }

            visible: {
                // The "improve your review" UI is app-specific and doesn't apply to KNS.
                if (reviewDialog.application.backend.name === "kns-backend") {
                    return false;
                }
                // Let people leave crappy reviews for proprietary apps, since trying to
                // communicate with their devs to improve them is usually like screaming
                // into the void.
                if (appHasAnyProprietaryLicenses) {
                    return false;
                }

                return reviewInput.hasAnySuspiciousKeywords && text.length > 0;
            }

            onLinkActivated: link => Qt.openUrlExternally(link)
        }

        Kirigami.Separator {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            Rating {
                id: ratingInput
                Kirigami.FormData.label: i18n("Rating:")
                readOnly: false
            }
            QQC2.TextField {
                id: nameInput
                Kirigami.FormData.label: i18n("Name:")
                visible: page.reviewsBackend !== null && reviewDialog.backend.preferredUserName.length > 0
                Layout.fillWidth: true
                readOnly: !reviewDialog.backend.supportsNameChange
                text: visible ? reviewDialog.backend.preferredUserName : ""
            }
            QQC2.TextField {
                id: titleInput
                Kirigami.FormData.label: i18n("Title:")
                Layout.fillWidth: true
                validator: RegularExpressionValidator { regularExpression: /.{3,70}/ }
            }
        }

        QQC2.TextArea {
            id: reviewInput

            readonly property string lowercaseText: reviewInput.text.toLowerCase()

            // Intentionally untranslated to cover the case of people writing reviews
            // in English when their system is in another language.
            readonly property string doesntRunKeywordsEn: "launch,run,start"
            readonly property string doesntRunKeywordsI18n: i18nc("Words the user might use in an unhelpful sentence saying an app is not launching. Preserve the commas. If necessary, add more words used for this in your language and/or shorten to common stems contained by multiple forms of the words.",
                                                                  "launch,run,start")
            readonly property list<string> doesntRunKeywords: doesntRunKeywordsEn.concat(",", doesntRunKeywordsI18n).split(",")
            readonly property bool hasDoesntRunKeywords: doesntRunKeywords.some(suspiciousWord => lowercaseText.includes(suspiciousWord))


            readonly property string crashKeywordsEn: "crash,segfault"
            readonly property string crashKeywordsI18n:  i18nc("Words the user might use in an unhelpful sentence saying an app crashes. Preserve the commas. If necessary, add more words used for this in your language and/or shorten to common stems contained by multiple forms of the words",
                                                               "crash,segfault")
            readonly property list<string> crashKeywords: crashKeywordsEn.concat(",", crashKeywordsI18n).split(",")
            readonly property bool hasCrashKeywords: crashKeywords.some(suspiciousWord => lowercaseText.includes(suspiciousWord))


            readonly property string subjectiveKeywordsEn: "doesn't work,dumb,stupid,crap,junk,suck,terrible,hate"
            readonly property string subjectiveKeywordsI18n: i18nc("Word the user might use in an unhelpful sentence saying an app isn't very good. Preserve the commas. If necessary, add more words used for this in your language and/or shorten to common stems contained by multiple forms of the words,",
                                                                   "doesn't work,dumb,stupid,crap,junk,suck,terrible,hate")
            readonly property list<string> subjectiveKeywords: subjectiveKeywordsEn.concat(",", subjectiveKeywordsI18n).split(",")
            readonly property bool hasSubjectiveKeywords: subjectiveKeywords.some(suspiciousWord => lowercaseText.includes(suspiciousWord))


            readonly property bool hasAnySuspiciousKeywords: hasDoesntRunKeywords || hasSubjectiveKeywords || hasCrashKeywords

            readonly property bool acceptableInput: length > 15 && length < 3000

            Layout.fillWidth: true
            Layout.minimumHeight: Kirigami.Units.gridUnit * 8
            KeyNavigation.priority: KeyNavigation.BeforeItem
            wrapMode: TextEdit.Wrap
        }

        QQC2.Label {
            id: instructionalLabel
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
            text: {
                if (rating < 2) {
                    return i18n("Enter a rating");
                }
                if (!titleInput.acceptableInput) {
                    return i18n("Write the title");
                }
                if (reviewInput.length === 0) {
                    return i18n("Write the review");
                }
                if (reviewInput.length < 15) {
                    return i18n("Keep writing…");
                }
                if (reviewInput.length > 3000) {
                    return i18n("Too long!");
                }
                if (nameInput.visible && nameInput.length < 1) {
                    return i18nc("@info:usagetip", "Insert a name");
                }
                return "";
            }
            wrapMode: Text.WordWrap
            opacity: 0.75
            visible: text.length > 0
        }
    }

    onOpened: {
        appHasAnyProprietaryLicenses = application.licenses.some(license => license.licenseType === "proprietary")

        ratingInput.forceActiveFocus(Qt.PopupFocusReason);
    }

    Component.onCompleted: {
        const submitButton = customFooterButton(submitAction);
        if (submitButton) {
            reviewInput.KeyNavigation.tab = submitButton;
            submitButton.KeyNavigation.tab = ratingInput;
        }
    }
}
