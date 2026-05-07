#include "PluginEditor.h"
#include <BinaryData.h>

WebPlugEditor::WebPlugEditor(WebPlugProcessor& p)
    : AudioProcessorEditor(&p)
{
    auto& proc = getWebPlugProcessor();

    auto options = juce::WebBrowserComponent::Options{}
        .withNativeFunction(
            "webplugMessage",
            [&proc](const juce::var& params,
                    juce::WebBrowserComponent::NativeFunctionCompletion complete)
            {
                proc.handleMessageFromUI(params.toString());
                complete({});
            });

    browser = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(browser.get());

    // Wire C++ → UI: called by processor to push messages into the browser.
    proc.onMessageToUI = [this](const juce::String& json) {
        juce::MessageManager::callAsync([this, json] {
            if (browser) {
                juce::String escaped = json.replace("\\", "\\\\").replace("'", "\\'");
                browser->evaluateJavascript("window.webplugReceive('" + escaped + "')");
            }
        });
    };

    // Write embedded UI files to temp dir and load via file://.
    // Custom URL schemes (juce://, app://) are unreliable across JUCE versions.
    juce::File uiDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("WebPlugUI");
    uiDir.createDirectory();

    struct UIFile { const char* name; const char* data; int size; };
    static const UIFile uiFiles[] = {
        { "index.html", WebPlugBinaryData::index_html,  WebPlugBinaryData::index_htmlSize  },
        { "styles.css", WebPlugBinaryData::styles_css,  WebPlugBinaryData::styles_cssSize  },
        { "bridge.js",  WebPlugBinaryData::bridge_js,   WebPlugBinaryData::bridge_jsSize   },
        { "main.js",    WebPlugBinaryData::main_js,     WebPlugBinaryData::main_jsSize     },
    };
    for (const auto& f : uiFiles)
        uiDir.getChildFile(f.name).replaceWithData(f.data, (size_t)f.size);

    juce::String devPath = juce::SystemStats::getEnvironmentVariable(
        "WEBPLUG_DEV_UI_PATH", {});

    if (devPath.isNotEmpty()) {
        juce::File devIndex = juce::File(devPath).getChildFile("index.html");
        browser->goToURL("file://" + (devIndex.existsAsFile()
            ? devIndex : uiDir.getChildFile("index.html")).getFullPathName());
    } else {
        browser->goToURL("file://" + uiDir.getChildFile("index.html").getFullPathName());
    }

    setSize(480, 320);
    setResizable(true, true);
}

WebPlugEditor::~WebPlugEditor()
{
    getWebPlugProcessor().onMessageToUI = nullptr;
}

void WebPlugEditor::resized()
{
    if (browser) browser->setBounds(getLocalBounds());
}
