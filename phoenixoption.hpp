#ifndef quantlib_phoenix_option_hpp
#define quantlib_phoenix_option_hpp

#include <ql/instruments/oneassetoption.hpp>
//#include <ql/instruments/barriertype.hpp>
#include <ql/instruments/payoffs.hpp>

//using namespace QuantLib;
namespace QuantLib {

    class GeneralizedBlackScholesProcess;

	class PhoenixOption : public OneAssetOption {
      public:
        class arguments;
        class engine;
        PhoenixOption(Real principal,
					  Real upbarrier,
					  Real downbarrier,
                      Real rebate,
                      const ext::shared_ptr<StrikedTypePayoff>& payoff,
                      const ext::shared_ptr<Exercise>& exercise);
        void setupArguments(PricingEngine::arguments*) const;
        /*! \warning see VanillaOption for notes on implied-volatility
                     calculation.
        */
        Volatility impliedVolatility(
             Real price,
             const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
             Real accuracy = 1.0e-4,
             Size maxEvaluations = 100,
             Volatility minVol = 1.0e-7,
             Volatility maxVol = 4.0) const;
      protected:
        // arguments
        Real principal_;
        Real upbarrier_;
		Real downbarrier_;
        Real rebate_;
    };

	    //! %Arguments for barrier option calculation
    class PhoenixOption::arguments : public OneAssetOption::arguments {
      public:
        arguments();
        Real principal_;
        Real upbarrier_;
		Real downbarrier_;
        Real rebate_;
        void validate() const;
    };

	//! %Barrier-option %engine base class
    class PhoenixOption::engine
        : public GenericEngine<PhoenixOption::arguments,
                               PhoenixOption::results> {
      protected:
        bool triggered(Real underlying) const;
    };



}

#endif