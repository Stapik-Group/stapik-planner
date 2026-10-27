#include "core/model/DragPayload.hpp"

#include <gtest/gtest.h>

TEST(DragPayloadTest, AnActivityPayloadSurvivesTheRoundTrip)
{
    const auto payload = DragPayload::forActivity("gym-1");

    EXPECT_EQ(DragPayload::deserialize(payload.serialize()), payload);
}

TEST(DragPayloadTest, ASlotPayloadSurvivesTheRoundTripWithItsCopyFlag)
{
    const auto moved = DragPayload::forSlot(Weekday::FRIDAY, 3, false);
    const auto copied = DragPayload::forSlot(Weekday::SUNDAY, 0, true);

    EXPECT_EQ(DragPayload::deserialize(moved.serialize()), moved);
    EXPECT_EQ(DragPayload::deserialize(copied.serialize()), copied);
}

TEST(DragPayloadTest, MalformedPayloadsAreRejected)
{
    EXPECT_FALSE(DragPayload::deserialize("").has_value());
    EXPECT_FALSE(DragPayload::deserialize("activity:").has_value());
    EXPECT_FALSE(DragPayload::deserialize("something else").has_value());
    EXPECT_FALSE(DragPayload::deserialize("slot:1:2").has_value());
    EXPECT_FALSE(DragPayload::deserialize("slot:7:0:0").has_value());
    EXPECT_FALSE(DragPayload::deserialize("slot:1:-1:0").has_value());
    EXPECT_FALSE(DragPayload::deserialize("slot:1:2:5").has_value());
    EXPECT_FALSE(DragPayload::deserialize("slot:a:2:0").has_value());
    EXPECT_FALSE(DragPayload::deserialize("slot:1:2:0:9").has_value());
}
