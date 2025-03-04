
#include "Southpole.hpp"

#define NUMAUX 2

struct Aux : Module {
  enum ParamIds {
    SEND1_PARAM,
    SEND2_PARAM,
    RETURN1_PARAM,
    RETURN2_PARAM,
    FEEDBACK1_PARAM,
    FEEDBACK2_PARAM,
    MUTE_PARAM,
    BYPASS_PARAM,
    NUM_PARAMS
  };
  enum InputIds {
    INL_INPUT,
    INR_INPUT,
    RETURN1L_INPUT,
    RETURN2L_INPUT,
    RETURN1R_INPUT,
    RETURN2R_INPUT,
    NUM_INPUTS
  };
  enum OutputIds {
    OUTL_OUTPUT,
    OUTR_OUTPUT,
    SEND1L_OUTPUT,
    SEND2L_OUTPUT,
    SEND1R_OUTPUT,
    SEND2R_OUTPUT,
    NUM_OUTPUTS
  };
  enum LightIds {
    MUTE_LIGHT,
    BYPASS_LIGHT,
    NUM_LIGHTS
  };

  dsp::SchmittTrigger muteTrigger;
  dsp::SchmittTrigger bypassTrigger;
  bool mute;
  bool bypass;

  Aux() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    mute = 0;
    bypass = 0;

    configParam(Aux::SEND1_PARAM, 0.0, 1.0, 0.5, "");
    configParam(Aux::RETURN1_PARAM, 0.0, 1.0, 0.5, "");
    configParam(Aux::FEEDBACK1_PARAM, -1.0, 1.0, 0.0, "");
    configParam(Aux::FEEDBACK2_PARAM, -1.0, 1.0, 0.0, "");
    configParam(Aux::SEND2_PARAM, 0.0, 1.0, 0.5, "");
    configParam(Aux::RETURN2_PARAM, 0.0, 1.0, 0.5, "");
    configParam(Aux::MUTE_PARAM, 0.0, 1.0, 0.0, "");
    configParam(Aux::BYPASS_PARAM, 0.0, 1.0, 0.0, "");
  }
  void process(const ProcessArgs &args) override;

  json_t *dataToJson() override {
    json_t *rootJm = json_object();
    json_t *statesJ = json_array();

    json_t *muteJ = json_boolean(mute);
    json_array_append_new(statesJ, muteJ);

    json_t *bypassJ = json_boolean(bypass);
    json_array_append_new(statesJ, bypassJ);

    json_object_set_new(rootJm, "states", statesJ);

    return rootJm;
  }

  void dataFromJson(json_t *rootJm) override {
    json_t *statesJ = json_object_get(rootJm, "states");

    json_t *muteJ = json_array_get(statesJ, 0);
    json_t *bypassJ = json_array_get(statesJ, 1);

    mute = !!json_boolean_value(muteJ);
    bypass = !!json_boolean_value(bypassJ);
  }
};

void Aux::process(const ProcessArgs &args) {

  if (muteTrigger.process(params[MUTE_PARAM].getValue())) {
    mute = !mute;
  }
  lights[MUTE_LIGHT].value = mute ? 1.0 : 0.0;

  if (bypassTrigger.process(params[BYPASS_PARAM].getValue())) {
    bypass = !bypass;
  }
  lights[BYPASS_LIGHT].value = bypass ? 1.0 : 0.0;

  float inl = 0.;
  float inr = 0.;

  if (!mute) {
    inl = inputs[INL_INPUT].getNormalVoltage(0.);
    inr = inputs[INR_INPUT].getNormalVoltage(inl);
  }

  float outl = inl;
  float outr = inr;

  float sl1 = params[SEND1_PARAM].getValue() * inl;
  float sr1 = params[SEND1_PARAM].getValue() * inr;

  float sl2 = params[SEND2_PARAM].getValue() * inl;
  float sr2 = params[SEND2_PARAM].getValue() * inr;

  float rl1 = inputs[RETURN1L_INPUT].getNormalVoltage(0.);
  float rr1 = inputs[RETURN1R_INPUT].getNormalVoltage(rl1);

  float rl2 = inputs[RETURN2L_INPUT].getNormalVoltage(0.);
  float rr2 = inputs[RETURN2R_INPUT].getNormalVoltage(rl2);

  float fb1 = params[FEEDBACK1_PARAM].getValue();
  float fb2 = params[FEEDBACK2_PARAM].getValue();

  if (fb1 >= 0.) {
    sl1 += fb1 * rl2;
    sr1 += fb1 * rr2;
  } else {
    sr1 -= fb1 * rl2;
    sl1 -= fb1 * rr2;
  }

  if (fb2 >= 0.) {
    sl2 += fb2 * rl1;
    sr2 += fb2 * rr1;
  } else {
    sr2 -= fb2 * rl1;
    sl2 -= fb2 * rr1;
  }

  outputs[SEND1L_OUTPUT].setVoltage(sl1);
  outputs[SEND1R_OUTPUT].setVoltage(sr1);

  outputs[SEND2L_OUTPUT].setVoltage(sl2);
  outputs[SEND2R_OUTPUT].setVoltage(sr2);

  if (!bypass) {
    outl += params[RETURN1_PARAM].getValue() * rl1;
    outr += params[RETURN1_PARAM].getValue() * rr1;
    outl += params[RETURN2_PARAM].getValue() * rl2;
    outr += params[RETURN2_PARAM].getValue() * rr2;
  }

  outputs[OUTL_OUTPUT].setVoltage(outl);
  outputs[OUTR_OUTPUT].setVoltage(outr);
}

struct AuxWidget : ModuleWidget {
  AuxWidget(Aux *module) {
    setModule(module);

    box.size = Vec(15 * 4, 380);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Aux_.svg")));

    const float y1 = 42;
    const float yh = 26;

    const float x1 = 4.;
    //const float x2 = 20.;
    const float x3 = 36.;

    addOutput(createOutput<sp_Port>(Vec(x1, y1 + 0 * yh), module, Aux::SEND1L_OUTPUT));
    addOutput(createOutput<sp_Port>(Vec(x1, y1 + 1 * yh), module, Aux::SEND1R_OUTPUT));
    addInput(createInput<sp_Port>(Vec(x3, y1 + 0 * yh), module, Aux::RETURN1L_INPUT));
    addInput(createInput<sp_Port>(Vec(x3, y1 + 1 * yh), module, Aux::RETURN1R_INPUT));
    addParam(createParam<sp_SmallBlackKnob>(Vec(x1, y1 + 2 * yh), module, Aux::SEND1_PARAM));
    addParam(createParam<sp_SmallBlackKnob>(Vec(x3, y1 + 2 * yh), module, Aux::RETURN1_PARAM));

    addParam(createParam<sp_Trimpot>(Vec(x1, y1 + 3.5 * yh), module, Aux::FEEDBACK1_PARAM));
    addParam(createParam<sp_Trimpot>(Vec(x3, y1 + 3.5 * yh), module, Aux::FEEDBACK2_PARAM));

    addOutput(createOutput<sp_Port>(Vec(x1, y1 + 5.5 * yh), module, Aux::SEND2L_OUTPUT));
    addOutput(createOutput<sp_Port>(Vec(x1, y1 + 6.5 * yh), module, Aux::SEND2R_OUTPUT));
    addInput(createInput<sp_Port>(Vec(x3, y1 + 5.5 * yh), module, Aux::RETURN2L_INPUT));
    addInput(createInput<sp_Port>(Vec(x3, y1 + 6.5 * yh), module, Aux::RETURN2R_INPUT));
    addParam(createParam<sp_SmallBlackKnob>(Vec(x1, y1 + 7.5 * yh), module, Aux::SEND2_PARAM));
    addParam(createParam<sp_SmallBlackKnob>(Vec(x3, y1 + 7.5 * yh), module, Aux::RETURN2_PARAM));

    addParam(createParam<LEDButton>(Vec(x1, y1 + 9 * yh), module, Aux::MUTE_PARAM));
    addChild(createLight<LargeLight<RedLight>>(Vec(x1 + 2.2, y1 + 9 * yh + 2), module, Aux::MUTE_LIGHT));

    addParam(createParam<LEDButton>(Vec(x3, y1 + 9 * yh), module, Aux::BYPASS_PARAM));
    addChild(createLight<LargeLight<RedLight>>(Vec(x3 + 2.2, y1 + 9 * yh + 2), module, Aux::BYPASS_LIGHT));

    addInput(createInput<sp_Port>(Vec(x1, y1 + 10 * yh), module, Aux::INL_INPUT));
    addInput(createInput<sp_Port>(Vec(x1, y1 + 11 * yh), module, Aux::INR_INPUT));

    addOutput(createOutput<sp_Port>(Vec(x3, y1 + 10 * yh), module, Aux::OUTL_OUTPUT));
    addOutput(createOutput<sp_Port>(Vec(x3, y1 + 11 * yh), module, Aux::OUTR_OUTPUT));
  }
};

Model *modelAux = createModel<Aux, AuxWidget>("Aux");
