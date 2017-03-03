/*
 * vending_msm_test.cpp
 *
 *  Created on: 20 нояб. 2016 г.
 *      Author: sergey.fedorov
 */

#include <gtest/gtest.h>
#include "vending_machine_msm.hpp"

namespace vending_msm {

TEST(VendingMSM, OnOff)
{
    vending_machine vm;

    EXPECT_EQ(0ul, vm.count()) << "No goods were loaded by default constructor";
    EXPECT_TRUE(vm.is_empty()) << "Vending machine is empty";

    EXPECT_TRUE(vm.is_flag_active< vending_def::off >())
                                << "Vending machine is off";
    EXPECT_FALSE(vm.is_flag_active< vending_def::on >())
                                << "Vending machine is not on";

    EXPECT_TRUE(vm.process_event(events::power_on{}))
                                << "Vending machine turns on correctly";

    EXPECT_FALSE(vm.is_flag_active< vending_def::off >())
                                << "Vending machine is not off";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on >())
                                << "Vending machine is on";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::out_of_service >())
                                << "Vending machine is out of service (empty)";
    EXPECT_FALSE(vm.is_flag_active< vending_def::on::serving >())
                                << "Vending machine is not in serving state (empty)";

    EXPECT_TRUE(vm.process_event(events::power_off{}))
                                << "Vending machine turns off correctly";

    EXPECT_TRUE(vm.is_flag_active< vending_def::off >())
                                << "Vending machine is off";
    EXPECT_FALSE(vm.is_flag_active< vending_def::on >())
                                << "Vending machine is not on";
}

TEST(VendingMSM, Maintenance)
{
    vending_machine vm;

    // Try turn to maintenance when off
    EXPECT_FALSE(vm.process_event(events::start_maintenance{100500}))
                                << "Vending machine allows maintenance only when on";
    EXPECT_TRUE(vm.is_flag_active< vending_def::off >())
                                << "Vending machine stays off";

    // Power on
    EXPECT_TRUE(vm.process_event(events::power_on{}))
                                << "Vending machine turns on correctly";

    // Try use an invalid code
    EXPECT_TRUE(vm.process_event(events::start_maintenance{100500}))
                                << "Vending machine rejects incorrect code";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on >())
                                << "Vending machine stays on";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::out_of_service >())
                                << "Vending machine stays in previous state";

    // Use a correct code
    EXPECT_TRUE(vm.process_event(events::start_maintenance{vending_machine::factory_code}))
                                << "Vending machine accepts factory code";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on >())
                                << "Vending machine stays on";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::maintenance >())
                                << "Vending machine transits to maintenance mode";

    // Exit maintenance
    EXPECT_TRUE(vm.process_event(events::end_maintenance{}))
                                << "Vending machine exits maintenance";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on >())
                                << "Vending machine stays on";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::out_of_service >())
                                << "Vending machine transits to out of service (empty)";

    // Use a correct code
    EXPECT_TRUE(vm.process_event(events::start_maintenance{vending_machine::factory_code}))
                                << "Vending machine accepts factory code";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::maintenance >())
                                << "Vending machine transits to maintenance mode";

    // Turn off while in maintenance mode
    EXPECT_TRUE(vm.process_event(events::power_off{}))
                                << "Vending machine turns off correctly";

    EXPECT_TRUE(vm.is_flag_active< vending_def::off >())
                                << "Vending machine is off";
    EXPECT_TRUE(vm.is_flag_active< vending_def::off >())
                                << "Vending machine is off";

    // Turn back on
    EXPECT_TRUE(vm.process_event(events::power_on{}))
                                << "Vending machine turns on correctly";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::maintenance >())
                                << "Vending machine transits to maintenance mode";

    EXPECT_TRUE(vm.is_flag_active< vending_def::on::maintenance::idle >())
                                << "Vending machine is in idle maintenance mode";
    // Load some goods
    EXPECT_TRUE(vm.process_event(events::load_goods{ 0, 10 }))
                                << "Vending machine consumes goods";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::maintenance::idle >())
                                << "Vending machine transits back to idle maintenance mode";
    EXPECT_FALSE(vm.is_empty());
    EXPECT_EQ(10, vm.count());
    EXPECT_TRUE(vm.process_event(events::load_goods{ 1, 100 }))
                                << "Vending machine consumes goods";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::maintenance::idle >())
                                << "Vending machine transits back to idle maintenance mode";
    EXPECT_FALSE(vm.is_empty());
    EXPECT_EQ(110, vm.count());
    EXPECT_FALSE(vm.prices_correct());

    // Try leave maintenance mode without setting prices
    EXPECT_FALSE(vm.process_event(events::end_maintenance{}));

    // Set prices
    EXPECT_TRUE(vm.process_event(events::set_price{ 0, 10.0 }))
                                << "Set price for item 0";
    EXPECT_TRUE(vm.process_event(events::set_price{ 1, 5.0 }))
                                << "Set price for item 1";
    EXPECT_FALSE(vm.is_empty());
    EXPECT_TRUE(vm.prices_correct());
    // Leave maintenance mode
    //EXPECT_EQ(result::process, vm.process_event(events::end_maintenance{}));
}

TEST(VendingMSM, BuyItem)
{
    vending_machine vm{ goods_storage{
        { 0, { 10, 15.0f } },
        { 1, { 100, 5.0f } }
    }};

    auto initial_count = vm.count();

    EXPECT_FALSE(vm.is_empty()) << "Vending machine is empty";

    EXPECT_TRUE(vm.is_flag_active< vending_def::off >())
                                << "Vending machine is off";
    EXPECT_FALSE(vm.is_flag_active< vending_def::on >())
                                << "Vending machine is not on";

    EXPECT_TRUE(vm.process_event(events::power_on{}))
                                << "Vending machine turns on correctly";

    EXPECT_FALSE(vm.is_flag_active< vending_def::off >())
                                << "Vending machine is not off";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on >())
                                << "Vending machine is on";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::serving >())
                                << "Vending machine is serving";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::serving::idle >())
                                << "Vending machine is serving";

    EXPECT_TRUE(vm.process_event(events::money{3}))
                                << "Vending machine accepts money";
    EXPECT_TRUE(vm.is_flag_active< vending_def::on::serving::active >())
                                << "Vending machine is serving";
    EXPECT_TRUE(vm.process_event(events::select_item{0}))
                                << "Vending machine doesn't allow dispensing when low balance";
    EXPECT_TRUE(vm.process_event(events::select_item{42}))
                                << "Vending machine doesn't allow dispensing non-existent items";
    EXPECT_TRUE(vm.process_event(events::money{5}))
                                << "Vending machine accepts money";
    EXPECT_TRUE(vm.process_event(events::select_item{1}))
                                << "Vending machine dispenses when enough money";
    EXPECT_EQ(vm.get_price(1), vm.balance)
                                << "Machine's balance increased by item's price";
    EXPECT_EQ(initial_count - 1, vm.count())  << "Item count decreased by 1";
}

}  /* namespace vending_msm */
