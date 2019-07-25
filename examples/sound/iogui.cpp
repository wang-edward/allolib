

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_ParameterMIDI.hpp"
#include "al/ui/al_HtmlInterfaceServer.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

// This example shows the guis created for audio IO and for the ParameterMIDI classes

class MyApp : public App
{
public:
    MyApp() {
        nav().pos(Vec3d(0, 0, 8));
        addCone(mMesh);
        mMesh.primitive(Mesh::LINES);

        // Disable mouse nav to avoid naving while changing gui controls.
        navControl().useMouse(false);
        // Connect MIDI CC # 1 to "Number" parameter
        parameterMidi.connectControl(Number, 1, 1);
        // Connect MIDI CC # 7 to "Gain" parameter
        parameterMidi.connectControl(Gain, 7, 1);
    }

    virtual void onCreate() override {
        imguiInit();
    }

    virtual void onDraw(Graphics &g) override
    {
        g.clear(0);
        // The Number parameter determines how many times the cone is drawn
        for (int i = 0; i < Number.get(); ++i) {
            g.pushMatrix();
            g.translate((i % 4) - 2, (i / 4) - 2, -5);
            g.draw(mMesh);
            g.popMatrix();
        }
        // Draw a GUI
        imguiBeginFrame();
        ParameterGUI::beginPanel("IO");
        // Draw an interface to Audio IO.
        // This enables starting and stopping audio as well as selecting
        // Audio device and its parameters
        ParameterGUI::drawAudioIO(&audioIO());
        // Draw an interface to the ParameterMIDI object
        //
        ParameterGUI::drawParameterMIDI(&parameterMidi);
        ParameterGUI::endPanel();
        imguiEndFrame();
        imguiDraw();
    }
    virtual void onSound(AudioIOData &io) override {
      float gain = Gain; // Just update once per block;
      while(io()) {
        // Play white noise on speaker 1
        io.out(0) = rnd::uniformS() * gain;
      }
    }

private:
    Parameter Number{ "Number", "", 1, "", 0, 16 };
    Parameter Gain{ "Gain", "", 0.1, "", 0.0, 0.2 };
    ParameterMIDI parameterMidi;

    Mesh mMesh;
};


int main(int argc, char *argv[])
{
    MyApp app;
    app.dimensions(800, 600);
    app.title("Presets GUI");
    app.fps(30);
    app.initAudio(44100, 256, 2, 2);
    app.start();
    return 0;
}
