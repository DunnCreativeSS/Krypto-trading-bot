#ifndef K_UNITS_H_
#define K_UNITS_H_

namespace K {
  TEST_CASE("mConnectivity") {
    mConnectivity on, off;
    SECTION("assigned") {
      REQUIRE_NOTHROW(on = mConnectivity::Connected);
      REQUIRE_NOTHROW(off = mConnectivity::Disconnected);
      SECTION("values") {
        REQUIRE_FALSE(!on);
        REQUIRE(!off);
        SECTION("combined") {
          REQUIRE(on  * on  == on);
          REQUIRE(on  * off == off);
          REQUIRE(off * on  == off);
          REQUIRE(off * off == off);
        }
      }
    }
  }

  TEST_CASE("mLevel") {
    mLevel level;
    SECTION("defaults") {
      REQUIRE_NOTHROW(level = mLevel());
      SECTION("empty") {
        REQUIRE(level.empty());
      }
    }
    SECTION("assigned") {
      REQUIRE_NOTHROW(level = mLevel(
        1234.56,
        0.12345678
      ));
      SECTION("values") {
        REQUIRE(level.price == 1234.56);
        REQUIRE(level.size == 0.12345678);
      }
      SECTION("not empty") {
        REQUIRE_FALSE(level.empty());
      }
      SECTION("to json") {
        REQUIRE(((json)level).dump() == "{"
          "\"price\":1234.56,"
          "\"size\":0.12345678"
        "}");
      }
      SECTION("clear") {
        REQUIRE_NOTHROW(level.clear());
        SECTION("values") {
          REQUIRE_FALSE(level.price);
          REQUIRE_FALSE(level.size);
        }
        SECTION("empty") {
          REQUIRE(level.empty());
        }
        SECTION("to json") {
          REQUIRE(((json)level).dump() == "{\"price\":0.0}");
        }
      }
    }
  }
  TEST_CASE("mLevels") {
    mLevels levels;
    SECTION("defaults") {
      REQUIRE_NOTHROW(levels = mLevels());
      SECTION("empty") {
        REQUIRE(levels.empty());
      }
    }
    SECTION("assigned") {
      REQUIRE_NOTHROW(levels = mLevels(
        { mLevel(1234.56, 0.12345678) },
        { mLevel(1234.57, 0.12345679) }
      ));
      SECTION("values") {
        REQUIRE(levels.spread() == Approx(0.01));
      }
      SECTION("not empty") {
        REQUIRE_FALSE(levels.empty());
      }
      SECTION("to json") {
        REQUIRE(((json)levels).dump() == "{"
          "\"asks\":[{\"price\":1234.57,\"size\":0.12345679}],"
          "\"bids\":[{\"price\":1234.56,\"size\":0.12345678}]"
        "}");
      }
      SECTION("clear") {
        REQUIRE_NOTHROW(levels.clear());
        SECTION("values") {
          REQUIRE_FALSE(levels.spread());
        }
        SECTION("empty") {
          REQUIRE(levels.empty());
        }
        SECTION("to json") {
          REQUIRE(((json)levels).dump() == "{"
            "\"asks\":[],"
            "\"bids\":[]"
          "}");
        }
      }
    }
  }
  TEST_CASE("mMarketLevels") {
    mMarketLevels levels;
    SECTION("defaults") {
      REQUIRE_NOTHROW(levels = mMarketLevels());
      SECTION("fair value") {
        REQUIRE_FALSE(levels.fairValue);
        REQUIRE(levels.stats.fairPrice.ratelimit(levels.fairValue));
        REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
          FAIL("send() while ratelimit() = true because val still is = 0");
        });
        REQUIRE_NOTHROW(levels.stats.fairPrice.mToScreen::refresh = []() {
          FAIL("refresh() while ratelimit() = true because val still is = 0");
        });
        REQUIRE_NOTHROW(levels.calcFairValue(0.01));
        REQUIRE_FALSE(levels.fairValue);
      }
    }
    SECTION("assigned") {
      REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
        REQUIRE(((json)levels.stats.fairPrice).dump() == "{\"price\":1234.55}");
      });
      REQUIRE_NOTHROW(levels.stats.fairPrice.mToScreen::refresh = []() {
        INFO("refresh()");
      });
      REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::BBO);
      REQUIRE_NOTHROW(levels.filterBidOrders[1234.52] += 0.23456789);
      REQUIRE_NOTHROW(levels.filterBidOrders[1234.55] += 0.01234567);
      REQUIRE_NOTHROW(levels.filterAskOrders[1234.69] += 0.01234568);
      REQUIRE_NOTHROW(levels.send_reset_filter(mLevels(
        { mLevel(1234.50, 0.12345678), mLevel(1234.55, 0.01234567) },
        { mLevel(1234.60, 1.23456789), mLevel(1234.69, 0.11234569) }
      ), 0.01));
      SECTION("filters") {
        REQUIRE(levels.filterBidOrders.size() == 2);
        REQUIRE(levels.filterAskOrders.size() == 1);
        REQUIRE(levels.bids.size() == 1);
        REQUIRE(levels.bids[0].price == 1234.50);
        REQUIRE(levels.bids[0].size  == 0.12345678);
        REQUIRE(levels.asks.size() == 2);
        REQUIRE(levels.asks[0].price == 1234.60);
        REQUIRE(levels.asks[0].size  == 1.23456789);
        REQUIRE(levels.asks[1].price == 1234.69);
        REQUIRE(levels.asks[1].size  == 0.10000001);
        REQUIRE(levels.unfiltered.bids.size() == 2);
        REQUIRE(levels.unfiltered.bids[0].price == 1234.50);
        REQUIRE(levels.unfiltered.bids[0].size  == 0.12345678);
        REQUIRE(levels.unfiltered.bids[1].price == 1234.55);
        REQUIRE(levels.unfiltered.bids[1].size  == 0.01234567);
        REQUIRE(levels.unfiltered.asks.size() == 2);
        REQUIRE(levels.unfiltered.asks[0].price == 1234.60);
        REQUIRE(levels.unfiltered.asks[0].size  == 1.23456789);
        REQUIRE(levels.unfiltered.asks[1].price == 1234.69);
        REQUIRE(levels.unfiltered.asks[1].size  == 0.11234569);
      }
      SECTION("fair value") {
        REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = []() {
          FAIL("send() while ratelimit() = true because val still is = val");
        });
        REQUIRE_NOTHROW(levels.stats.fairPrice.mToScreen::refresh = []() {
          FAIL("refresh() while ratelimit() = true because val still is = val");
        });
        REQUIRE_NOTHROW(levels.calcFairValue(0.01));
        REQUIRE(levels.fairValue == 1234.55);
      }
      SECTION("fair value weight") {
        REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::wBBO);
        REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
          REQUIRE(((json)levels.stats.fairPrice).dump() == "{\"price\":1234.59}");
        });
        REQUIRE_NOTHROW(levels.calcFairValue(0.01));
        REQUIRE(levels.fairValue == 1234.59);
      }
      SECTION("fair value reversed weight") {
        REQUIRE_NOTHROW(qp.fvModel = mFairValueModel::rwBBO);
        REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
          REQUIRE(((json)levels.stats.fairPrice).dump() == "{\"price\":1234.51}");
        });
        REQUIRE_NOTHROW(levels.calcFairValue(0.01));
        REQUIRE(levels.fairValue == 1234.51);
      }
      SECTION("diff unset") {
        REQUIRE(levels.diff.ratelimit());
        REQUIRE(levels.diff.empty());
      }
      SECTION("diff reset") {
        REQUIRE(levels.diff.hello().dump() == "[{"
          "\"asks\":[{\"price\":1234.6,\"size\":1.23456789},{\"price\":1234.69,\"size\":0.11234569}],"
          "\"bids\":[{\"price\":1234.5,\"size\":0.12345678},{\"price\":1234.55,\"size\":0.01234567}]"
        "}]");
        REQUIRE_FALSE(levels.diff.ratelimit());
        REQUIRE_FALSE(levels.diff.empty());
        REQUIRE_FALSE(levels.diff.patched);
        SECTION("diff send") {
          REQUIRE_NOTHROW(levels.diff.mToClient::send = [&]() {
            REQUIRE(levels.diff.patched);
            REQUIRE(((json)levels.diff).dump() == "{"
              "\"asks\":[{\"price\":1234.69,\"size\":0.11234566}],"
              "\"bids\":[{\"price\":1234.5},{\"price\":1234.4,\"size\":0.12345678}],"
              "\"diff\":true"
            "}");
          });
          REQUIRE_NOTHROW(levels.stats.fairPrice.mToClient::send = [&]() {
            REQUIRE(((json)levels.stats.fairPrice).dump() == "{\"price\":1234.5}");
          });
          REQUIRE_NOTHROW(levels.send_reset_filter(mLevels(
            { mLevel(1234.40, 0.12345678), mLevel(1234.55, 0.01234567) },
            { mLevel(1234.60, 1.23456789), mLevel(1234.69, 0.11234566) }
          ), 0.01));
          REQUIRE_FALSE(levels.diff.patched);
          REQUIRE(((json)levels.diff).dump() == "{"
            "\"asks\":[{\"price\":1234.6,\"size\":1.23456789},{\"price\":1234.69,\"size\":0.11234566}],"
            "\"bids\":[{\"price\":1234.4,\"size\":0.12345678},{\"price\":1234.55,\"size\":0.01234567}]"
          "}");
        }
      }
    }
  }

  TEST_CASE("mRecentTrades") {
    mRecentTrades recentTrades;
    SECTION("defaults") {
      REQUIRE_NOTHROW(recentTrades = mRecentTrades());
    }
    SECTION("assigned") {
      REQUIRE_NOTHROW(recentTrades.insert(mSide::Ask, 1234.57, 0.01234566));
      REQUIRE_NOTHROW(recentTrades.insert(mSide::Ask, 1234.58, 0.01234567));
      REQUIRE_NOTHROW(recentTrades.insert(mSide::Bid, 1234.56, 0.12345678));
      REQUIRE_NOTHROW(recentTrades.insert(mSide::Bid, 1234.50, 0.12345679));
      REQUIRE_NOTHROW(recentTrades.insert(mSide::Ask, 1234.60, 0.12345678));
      SECTION("values") {
        REQUIRE(recentTrades.lastBuyPrice == 1234.50);
        REQUIRE(recentTrades.lastSellPrice == 1234.60);
        REQUIRE(recentTrades.buys.size() == 2);
        REQUIRE(recentTrades.sells.size() == 3);
        REQUIRE_FALSE(recentTrades.sumBuys);
        REQUIRE_FALSE(recentTrades.sumSells);
        SECTION("reset") {
          SECTION("skip") {
            REQUIRE_NOTHROW(recentTrades.reset());
            REQUIRE(recentTrades.lastBuyPrice == 1234.50);
            REQUIRE(recentTrades.lastSellPrice == 1234.60);
            REQUIRE(recentTrades.buys.size() == 1);
            REQUIRE_FALSE(recentTrades.sells.size());
            REQUIRE(recentTrades.sumBuys == 0.09876546);
            REQUIRE_FALSE(recentTrades.sumSells);
          }
          SECTION("expired") {
            this_thread::sleep_for(chrono::milliseconds(1001));
            REQUIRE_NOTHROW(qp.tradeRateSeconds = 1);
            REQUIRE_NOTHROW(recentTrades.reset());
            REQUIRE(recentTrades.lastBuyPrice == 1234.50);
            REQUIRE(recentTrades.lastSellPrice == 1234.60);
            REQUIRE_FALSE(recentTrades.buys.size());
            REQUIRE_FALSE(recentTrades.sells.size());
            REQUIRE_FALSE(recentTrades.sumBuys);
            REQUIRE_FALSE(recentTrades.sumSells);
          }
        }
      }
    }
  }
}

#endif