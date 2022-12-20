
#include <initializer_list>
#include <unordered_set>

#include <catch2/catch_test_macros.hpp>

#include <uwb/protocols/fira/RangingConfiguration.hxx>

using namespace uwb::protocol::fira;

TEST_CASE("RangingConfiguration can be used in unordered_containers", "[basic]")
{
    // Set with all possible combinations of object properties.
    const std::initializer_list<RangingConfiguration> RangingConfigurationsAll = {
        RangingConfiguration{ RangingDirection::OneWay, MeasurementReportMode::None },
        RangingConfiguration{ RangingDirection::SingleSidedTwoWay, MeasurementReportMode::None },
        RangingConfiguration{ RangingDirection::DoubleSidedTwoWay, MeasurementReportMode::None },
        RangingConfiguration{ RangingDirection::OneWay, MeasurementReportMode::Deferred },
        RangingConfiguration{ RangingDirection::SingleSidedTwoWay, MeasurementReportMode::Deferred },
        RangingConfiguration{ RangingDirection::DoubleSidedTwoWay, MeasurementReportMode::Deferred },
        RangingConfiguration{ RangingDirection::OneWay, MeasurementReportMode::NonDeferred },
        RangingConfiguration{ RangingDirection::SingleSidedTwoWay, MeasurementReportMode::NonDeferred },
        RangingConfiguration{ RangingDirection::DoubleSidedTwoWay, MeasurementReportMode::NonDeferred },
    };

    SECTION("can be used in std::unordered_set")
    {
        std::unordered_set<RangingConfiguration> rangingConfigurations{};

        // Populate set with initial objects, ensure all were inserted, implying no existing element.
        for (const auto& rangingConfiguration : RangingConfigurationsAll) {
            auto [_, inserted] = rangingConfigurations.insert(rangingConfiguration);
            REQUIRE(inserted);
        }

        // Populate set with same objects, ensure none were inserted, implying elements already exist.
        for (const auto& rangingConfiguration : RangingConfigurationsAll) {
            auto [_, inserted] = rangingConfigurations.insert(rangingConfiguration);
            REQUIRE(!inserted);
        }
    }

    SECTION("can be used as std::unordered_map key")
    {
        unsigned value = 0;
        std::unordered_map<RangingConfiguration, unsigned> rangingConfigurations{};

        // Populate map with initial objects, ensure all were inserted, implying no existing element.
        for (const auto& rangingConfiguration : RangingConfigurationsAll) {
            auto [_, inserted] = rangingConfigurations.insert({ rangingConfiguration, value++ });
            REQUIRE(inserted);
        }

        // Reset value such that insertion sequence matches the one above.
        value = 0;

        // Populate map with same objects, ensure none were inserted, implying elements already exist.
        for (const auto& rangingConfiguration : RangingConfigurationsAll) {
            auto [_, inserted] = rangingConfigurations.insert({ rangingConfiguration, value++ });
            REQUIRE(!inserted);
        }
    }
}
