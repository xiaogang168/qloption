#include "phoenixoption.hpp"
#include <ql/instruments/impliedvolatility.hpp>
#include "mcphoenixengine.hpp"
#include <ql/exercise.hpp>
#include <boost/scoped_ptr.hpp>

using namespace QuantLib;
//namespace QuantLib {

    PhoenixOption::PhoenixOption(
        Real principal,
		Real upbarrier,
		Real downbarrier,
        Real rebate,
        const ext::shared_ptr<StrikedTypePayoff>& payoff,
        const ext::shared_ptr<Exercise>& exercise)
    : OneAssetOption(payoff, exercise),
      principal_(principal), upbarrier_(upbarrier), downbarrier_(downbarrier), rebate_(rebate) {}

    void PhoenixOption::setupArguments(PricingEngine::arguments* args) const {

        OneAssetOption::setupArguments(args);

        PhoenixOption::arguments* moreArgs =
            dynamic_cast<PhoenixOption::arguments*>(args);
        QL_REQUIRE(moreArgs != 0, "wrong argument type");
        moreArgs->principal_ = principal_;
        moreArgs->upbarrier_ = upbarrier_;
		moreArgs->downbarrier_ = downbarrier_;
        moreArgs->rebate_ = rebate_;
    }


    Volatility PhoenixOption::impliedVolatility(
             Real price,
             const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
             Real accuracy,
             Size maxEvaluations,
             Volatility minVol,
             Volatility maxVol) const {

        QL_REQUIRE(!isExpired(), "option expired");

        ext::shared_ptr<SimpleQuote> volQuote(new SimpleQuote);

        ext::shared_ptr<GeneralizedBlackScholesProcess> newProcess =
            detail::ImpliedVolatilityHelper::clone(process, volQuote);

        // engines are built-in for the time being
        boost::scoped_ptr<PricingEngine> engine;
        switch (exercise_->type()) {
          case Exercise::European:
            //engine.reset(new MCPhoenixEngine(newProcess));
            break;
          case Exercise::American:
			QL_FAIL("engine not available for non-European barrier option");
            break;
          case Exercise::Bermudan:
            QL_FAIL("engine not available for non-European barrier option");
            break;
          default:
            QL_FAIL("unknown exercise type");
        }

        return detail::ImpliedVolatilityHelper::calculate(*this,
                                                          *engine,
                                                          *volQuote,
                                                          price,
                                                          accuracy,
                                                          maxEvaluations,
                                                          minVol, maxVol);
    }


    PhoenixOption::arguments::arguments()
    : principal_(Null<Real>()), upbarrier_(Null<Real>()), downbarrier_(Null<Real>()),
      rebate_(Null<Real>()) {}

    void PhoenixOption::arguments::validate() const {
        OneAssetOption::arguments::validate();

		QL_REQUIRE(principal_ != Null<Real>(), "no principal given");
		QL_REQUIRE(upbarrier_ != Null<Real>(), "no upbarrier given");
        QL_REQUIRE(downbarrier_ != Null<Real>(), "no downbarrier given");
        QL_REQUIRE(rebate_ != Null<Real>(), "no rebate given");
    }

    bool PhoenixOption::engine::triggered(Real underlying) const {
        //switch (arguments_.barrierType) {
        //  case Barrier::DownIn:
        //  case Barrier::DownOut:
        //    return underlying < arguments_.barrier;
        //  case Barrier::UpIn:
        //  case Barrier::UpOut:
        //    return underlying > arguments_.barrier;
        //  default:
        //    QL_FAIL("unknown type");
        //}
		return true;
    }

//}