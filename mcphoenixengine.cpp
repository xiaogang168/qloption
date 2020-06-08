#include "mcphoenixengine.hpp"

using namespace QuantLib;
//namespace QuantLib {

    PhoenixPathPricer::PhoenixPathPricer(
					Real principal,
					Real upbarrier,
					Real downbarrier,
                    Real rebate,
                    Option::Type type,
                    Real strike,
                    const std::vector<DiscountFactor>& discounts,
                    const ext::shared_ptr<StochasticProcess1D>& diffProcess,
                    const PseudoRandom::ursg_type& sequenceGen)
					:principal_(principal), upbarrier_(upbarrier), downbarrier_(downbarrier),
      rebate_(rebate), diffProcess_(diffProcess),
      sequenceGen_(sequenceGen), payoff_(type, strike),
      discounts_(discounts) {
        QL_REQUIRE(strike>=0.0,
                   "strike less than zero not allowed");
        QL_REQUIRE(upbarrier>0.0,
                   "Phoenix less/equal zero not allowed");
    }


    Real PhoenixPathPricer::operator()(const Path& path) const {
        static Size null = Null<Size>();
        Size n = path.length();
        QL_REQUIRE(n>1, "the path cannot be empty");

        bool isDownActive = false;
		bool isUpActive = false;
		Size knockNode = null;
        Real asset_price = path.front();
        Real new_asset_price;
        Real x, y;
        Volatility vol;
        TimeGrid timeGrid = path.timeGrid();
        Time dt;
        std::vector<Real> u = sequenceGen_.nextSequence().value;
        Size i;

        for (i = 0; i < n-1; i++) {
            new_asset_price = path[i+1];
            // terminal or initial vol?
            vol = diffProcess_->diffusion(timeGrid[i],asset_price);
            dt = timeGrid.dt(i);

            x = std::log(new_asset_price / asset_price);
            y = 0.5*(x - std::sqrt (x*x - 2*vol*vol*dt*std::log(u[i])));
            y = asset_price * std::exp(y);
            if (y > upbarrier_) {
                isUpActive = true;
                if (knockNode == null){
                    knockNode = i+1;
				}
				break;
			}else if(y <= downbarrier_){
				isDownActive = true;
				knockNode = 0;  // 这里是否需要重新开始计算存续期，要看产品规则
			}else{
				if (knockNode == null){
                    knockNode = i+1;
				}else{
					++knockNode;
				}
				
			}

            asset_price = new_asset_price;
        }

		if(isDownActive){
			if(!isUpActive){
				return path.back() - path.front();
			}else{
			return rebate_ * principal_ * discounts_[knockNode];
			}

		}else{
			return rebate_ * principal_ * discounts_[knockNode];
		}
    }
//}