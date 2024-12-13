#include <main.hpp>

#include <gtest/gtest.h>

TEST(insert, matching_engine) {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,AAPL,BUY,12.2,5");

    auto result = run(input);

    EXPECT_TRUE(result.size() == 2);
    EXPECT_TRUE(result[0] == "===AAPL===");
    EXPECT_TRUE(result[1] == "12.2,5,,");
}

TEST(simple_match_sell_is_aggressive, matching_engine) {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,AAPL,BUY,12.2,5");
    input.emplace_back("INSERT,2,AAPL,SELL,12.1,8");

    auto result = run(input);

    EXPECT_TRUE(result.size() == 3);
    EXPECT_TRUE(result[0] == "AAPL,12.2,5,2,1");
    EXPECT_TRUE(result[1] == "===AAPL===");
    EXPECT_TRUE(result[2] == ",,12.1,3");
}


TEST(simple_match_buy_is_aggressive, matching_engine) {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,AAPL,SELL,12.1,8");
    input.emplace_back("INSERT,2,AAPL,BUY,12.2,5");

    auto result = run(input);

    EXPECT_TRUE(result.size() == 3);
    EXPECT_TRUE(result[0] == "AAPL,12.1,5,2,1");
    EXPECT_TRUE(result[1] == "===AAPL===");
    EXPECT_TRUE(result[2] == ",,12.1,3");
}




TEST(multi_symbol, matching_engine) {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,WEBB,BUY,0.3854,5");
    input.emplace_back("INSERT,2,TSLA,BUY,412,31");
    input.emplace_back("INSERT,3,TSLA,BUY,410.5,27");
    input.emplace_back("INSERT,4,AAPL,SELL,21,8");
    input.emplace_back("INSERT,11,WEBB,SELL,0.3854,4");
    input.emplace_back("INSERT,13,WEBB,SELL,0.3853,6");


    auto result = run(input);

    EXPECT_TRUE(result.size() == 9);
    EXPECT_TRUE(result[0] == "WEBB,0.3854,4,11,1");
    EXPECT_TRUE(result[1] == "WEBB,0.3854,1,13,1");
    EXPECT_TRUE(result[2] == "===AAPL===");
    EXPECT_TRUE(result[3] == ",,21,8");
    EXPECT_TRUE(result[4] == "===TSLA===");
    EXPECT_TRUE(result[5] == "412,31,,");
    EXPECT_TRUE(result[6] == "410.5,27,,");
    EXPECT_TRUE(result[7] == "===WEBB===");
    EXPECT_TRUE(result[8] == ",,0.3853,5");
}


TEST(multi_insert_and_multi_match, matching_engine) {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,8,AAPL,BUY,14.235,5");
    input.emplace_back("INSERT,6,AAPL,BUY,14.235,6");
    input.emplace_back("INSERT,7,AAPL,BUY,14.235,12");
    input.emplace_back("INSERT,2,AAPL,BUY,14.234,5");
    input.emplace_back("INSERT,1,AAPL,BUY,14.23,3");
    input.emplace_back("INSERT,5,AAPL,SELL,14.237,8");
    input.emplace_back("INSERT,3,AAPL,SELL,14.24,9");
    input.emplace_back("PULL,8");
    input.emplace_back("INSERT,4,AAPL,SELL,14.234,25");

    auto result = run(input);

    EXPECT_TRUE(result.size() == 7);
    EXPECT_TRUE(result[0] == "AAPL,14.235,6,4,6");
    EXPECT_TRUE(result[1] == "AAPL,14.235,12,4,7");
    EXPECT_TRUE(result[2] == "AAPL,14.234,5,4,2");
    EXPECT_TRUE(result[3] == "===AAPL===");
    EXPECT_TRUE(result[4] == "14.23,3,14.234,2");
    EXPECT_TRUE(result[5] == ",,14.237,8");
    EXPECT_TRUE(result[6] == ",,14.24,9");
}

TEST(amend, matching_engine) { //14
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,WEBB,BUY,45.95,5");
    input.emplace_back("INSERT,2,WEBB,BUY,45.95,6");
    input.emplace_back("INSERT,3,WEBB,BUY,45.95,12");
    input.emplace_back("INSERT,4,WEBB,SELL,46,8");
    input.emplace_back("AMEND,2,46,3");
    input.emplace_back("INSERT,5,WEBB,SELL,45.95,1");
    input.emplace_back("AMEND,1,45.95,3");
    input.emplace_back("INSERT,6,WEBB,SELL,45.95,1");
    input.emplace_back("AMEND,1,45.95,5");
    input.emplace_back("INSERT,7,WEBB,SELL,45.95,1");

    auto result = run(input);

    EXPECT_TRUE(result.size() == 6);
    EXPECT_TRUE(result[0] == "WEBB,46,3,2,4");
    EXPECT_TRUE(result[1] == "WEBB,45.95,1,5,1");
    EXPECT_TRUE(result[2] == "WEBB,45.95,1,6,1");
    EXPECT_TRUE(result[3] == "WEBB,45.95,1,7,3");
    EXPECT_TRUE(result[4] == "===WEBB===");
    EXPECT_TRUE(result[5] == "45.95,16,46,5");
}
