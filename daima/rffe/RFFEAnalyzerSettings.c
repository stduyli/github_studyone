#include "RFFEAnalyzerSettings.h"
#include <AnalyzerHelpers.h>

typedef struct {
    Channel mSclkChannel;
    Channel mSdataChannel;
    bool mShowParityInReport;
    bool mShowBusParkInReport;

    AnalyzerSettingInterfaceChannel* mSclkChannelInterface;
    AnalyzerSettingInterfaceChannel* mSdataChannelInterface;
    AnalyzerSettingInterfaceBool* mShowParityInReportInterface;
    AnalyzerSettingInterfaceBool* mShowBusParkInReportInterface;
} RFFEAnalyzerSettings;

void RFFEAnalyzerSettings_Init(RFFEAnalyzerSettings* settings) {
    settings->mSclkChannel = UNDEFINED_CHANNEL;
    settings->mSdataChannel = UNDEFINED_CHANNEL;

    settings->mSclkChannelInterface = new AnalyzerSettingInterfaceChannel();
    settings->mSclkChannelInterface->SetTitleAndTooltip("SCLK", "Specify the SCLK Signal(RFFEv1.10a)");
    settings->mSclkChannelInterface->SetChannel(settings->mSclkChannel);
    AddInterface(settings->mSclkChannelInterface);

    settings->mSdataChannelInterface = new AnalyzerSettingInterfaceChannel();
    settings->mSdataChannelInterface->SetTitleAndTooltip("SDATA", "Specify the SDATA Signal(RFFEv1.10a)");
    settings->mSdataChannelInterface->SetChannel(settings->mSdataChannel);
    AddInterface(settings->mSdataChannelInterface);

    settings->mShowParityInReportInterface = new AnalyzerSettingInterfaceBool();
    settings->mShowParityInReportInterface->SetTitleAndTooltip("Show Parity in Report?", "Check if you want parity information in the exported file");
    AddInterface(settings->mShowParityInReportInterface);

    settings->mShowBusParkInReportInterface = new AnalyzerSettingInterfaceBool();
    settings->mShowBusParkInReportInterface->SetTitleAndTooltip("Show BusPark in Report?", "Check if you want bus park information in the exported file");
    AddInterface(settings->mShowBusParkInReportInterface);

    AddExportOption(0, "Export as csv/text file");
    AddExportExtension(0, "csv", "csv");
    AddExportExtension(0, "text", "txt");

    ClearChannels();
    AddChannel(settings->mSclkChannel, "SCLK", false);
    AddChannel(settings->mSdataChannel, "SDATA", false);
}

bool RFFEAnalyzerSettings_SetSettingsFromInterfaces(RFFEAnalyzerSettings* settings) {
    settings->mSclkChannel = settings->mSclkChannelInterface->GetChannel();
    settings->mSdataChannel = settings->mSdataChannelInterface->GetChannel();
    settings->mShowParityInReport = settings->mShowParityInReportInterface->GetValue();
    settings->mShowBusParkInReport = settings->mShowBusParkInReportInterface->GetValue();

    ClearChannels();
    AddChannel(settings->mSclkChannel, "SCLK", true);
    AddChannel(settings->mSdataChannel, "SDATA", true);

    return true;
}

void RFFEAnalyzerSettings_UpdateInterfacesFromSettings(RFFEAnalyzerSettings* settings) {
    settings->mSclkChannelInterface->SetChannel(settings->mSclkChannel);
    settings->mSdataChannelInterface->SetChannel(settings->mSdataChannel);
    settings->mShowParityInReportInterface->SetValue(settings->mShowParityInReport);
    settings->mShowBusParkInReportInterface->SetValue(settings->mShowBusParkInReport);
}

void RFFEAnalyzerSettings_LoadSettings(RFFEAnalyzerSettings* settings, const char* settingsString) {
    SimpleArchive text_archive;
    text_archive.SetString(settingsString);

    text_archive >> settings->mSclkChannel;
    text_archive >> settings->mSdataChannel;
    text_archive >> settings->mShowParityInReport;
    text_archive >> settings->mShowBusParkInReport;

    ClearChannels();
    AddChannel(settings->mSclkChannel, "SCLK", true);
    AddChannel(settings->mSdataChannel, "SDATA", true);

    RFFEAnalyzerSettings_UpdateInterfacesFromSettings(settings);
}

const char* RFFEAnalyzerSettings_SaveSettings(RFFEAnalyzerSettings* settings) {
    SimpleArchive text_archive;

    text_archive << settings->mSclkChannel;
    text_archive << settings->mSdataChannel;
    text_archive << settings->mShowParityInReport;
    text_archive << settings->mShowBusParkInReport;

    return SetReturnString(text_archive.GetString());
}
