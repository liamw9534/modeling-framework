#pragma once
#include<vector>
#include "ModelingFramework.h"

class ADG1404 : public ExternalPeripheral {
  public:
    ADG1404();
    void Main() override;
    void Stop() override;

    ADG1404(const ADG1404&) = delete;
    ADG1404& operator=(const ADG1404&) = delete;

  private:
    iShort* adg1404_short {};
    std::vector<uint32_t> source_pin_{};
    void SetAdg1404Short();
    void OnPinChangeLevelEvent(std::vector<WireLogicLevelEvent>& notifications);
};

DLL_EXPORT ExternalPeripheral *PeripheralFactory() {
    return new ADG1404();
}
