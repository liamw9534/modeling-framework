#include "ADG1404.h"

ADG1404::ADG1404() {
    source_pin_.push_back(GetPinNumber("pinS1"));
    source_pin_.push_back(GetPinNumber("pinS2"));
    source_pin_.push_back(GetPinNumber("pinS3"));
    source_pin_.push_back(GetPinNumber("pinS4"));

    SetPinChangeLevelEventCallback("pinA0", std::bind(&ADG1404::OnPinChangeLevelEvent,this,std::placeholders::_1));
    SetPinChangeLevelEventCallback("pinA1", std::bind(&ADG1404::OnPinChangeLevelEvent,this,std::placeholders::_1));
    SetPinChangeLevelEventCallback("pinEn", std::bind(&ADG1404::OnPinChangeLevelEvent,this,std::placeholders::_1));

    adg1404_short = CreateShort();
    SetAdg1404Short();
}

void ADG1404::OnPinChangeLevelEvent(std::vector<WireLogicLevelEvent>& notifications) {
    SetAdg1404Short();
}

void ADG1404::SetAdg1404Short() {
    if (!GetPinLevel("pinEn")) {
        //enable pin is low, all switches are off
        adg1404_short->DisableShort();
    } else {
        // Choose the source pin according to A0 and A1
        int is_high_a0 = GetPinLevel("pinA0") ? 1 : 0;
        int is_high_a1 = GetPinLevel("pinA1") ? 1 : 0;
        int source_pin_number = is_high_a0 | (is_high_a1 << 1);

        adg1404_short->SetPin(ShortPinNumber::PIN1, source_pin_.at((unsigned long) source_pin_number));
        adg1404_short->SetPin(ShortPinNumber::PIN0, GetPinNumber("pinOutput"));
    }
}

void ADG1404::Main() {
}

void ADG1404::Stop() {
}