#ifndef quantlib_mc_phoenix_engines_hpp
#define quantlib_mc_phoenix_engines_hpp

#include <ql/instruments/barrieroption.hpp>
#include <ql/pricingengines/mcsimulation.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/exercise.hpp>
#include "phoenixoption.hpp"

using namespace QuantLib;
//namespace QuantLib {


	template <class RNG = PseudoRandom, class S = Statistics>
    class MCPhoenixEngine : public PhoenixOption::engine,
                            public McSimulation<SingleVariate,RNG,S> {
      public:
        typedef
        typename McSimulation<SingleVariate,RNG,S>::path_generator_type
            path_generator_type;
        typedef typename McSimulation<SingleVariate,RNG,S>::path_pricer_type
            path_pricer_type;
        typedef typename McSimulation<SingleVariate,RNG,S>::stats_type
            stats_type;
        // constructor
        MCPhoenixEngine(
             const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
             Size timeSteps,
             Size timeStepsPerYear,
             bool brownianBridge,
             bool antitheticVariate,
             Size requiredSamples,
             Real requiredTolerance,
             Size maxSamples,
             bool isBiased,
             BigNatural seed);
        void calculate() const {
            Real spot = process_->x0();
            QL_REQUIRE(spot >= 0.0, "negative or null underlying given");
            //QL_REQUIRE(!triggered(spot), "barrier touched");
            McSimulation<SingleVariate,RNG,S>::calculate(requiredTolerance_,
                                                         requiredSamples_,
                                                         maxSamples_);
            results_.value = this->mcModel_->sampleAccumulator().mean();
            if (RNG::allowsErrorEstimate)
            results_.errorEstimate =
                this->mcModel_->sampleAccumulator().errorEstimate();
        }
      protected:
        // McSimulation implementation
        TimeGrid timeGrid() const;
        ext::shared_ptr<path_generator_type> pathGenerator() const {
            TimeGrid grid = timeGrid();
            typename RNG::rsg_type gen =
                RNG::make_sequence_generator(grid.size()-1,seed_);
            return ext::shared_ptr<path_generator_type>(
                         new path_generator_type(process_,
                                                 grid, gen, brownianBridge_));
        }
        ext::shared_ptr<path_pricer_type> pathPricer() const;
        // data members
        ext::shared_ptr<GeneralizedBlackScholesProcess> process_;
        Size timeSteps_, timeStepsPerYear_;
        Size requiredSamples_, maxSamples_;
        Real requiredTolerance_;
        bool isBiased_;
        bool brownianBridge_;
        BigNatural seed_;
    };

	//! Monte Carlo phoenix-option engine factory
    template <class RNG = PseudoRandom, class S = Statistics>
    class MakeMCPhoenixEngine {
      public:
        MakeMCPhoenixEngine(
                    const ext::shared_ptr<GeneralizedBlackScholesProcess>&);
        // named parameters
        MakeMCPhoenixEngine& withSteps(Size steps);
        MakeMCPhoenixEngine& withStepsPerYear(Size steps);
        MakeMCPhoenixEngine& withBrownianBridge(bool b = true);
        MakeMCPhoenixEngine& withAntitheticVariate(bool b = true);
        MakeMCPhoenixEngine& withSamples(Size samples);
        MakeMCPhoenixEngine& withAbsoluteTolerance(Real tolerance);
        MakeMCPhoenixEngine& withMaxSamples(Size samples);
        MakeMCPhoenixEngine& withBias(bool b = true);
        MakeMCPhoenixEngine& withSeed(BigNatural seed);
        // conversion to pricing engine
        operator ext::shared_ptr<PricingEngine>() const;
      private:
        ext::shared_ptr<GeneralizedBlackScholesProcess> process_;
        bool brownianBridge_, antithetic_, biased_;
        Size steps_, stepsPerYear_, samples_, maxSamples_;
        Real tolerance_;
        BigNatural seed_;
    };

	class PhoenixPathPricer : public PathPricer<Path> {
		public:
			PhoenixPathPricer(
						Real principal,
						Real upbarrier,
						Real downbarrier,
						Real rebate,
						Option::Type type,
						Real strike,
						const std::vector<DiscountFactor>& discounts,
						const ext::shared_ptr<StochasticProcess1D>& diffProcess,
						const PseudoRandom::ursg_type& sequenceGen);
			Real operator()(const Path& path) const;
		private:
			Real principal_;
			Real upbarrier_;
			Real downbarrier_;
			Real rebate_;
			ext::shared_ptr<StochasticProcess1D> diffProcess_;
			PseudoRandom::ursg_type sequenceGen_;
			PlainVanillaPayoff payoff_;
			std::vector<DiscountFactor> discounts_;
	};

    // template definitions

    template <class RNG, class S>
    inline MCPhoenixEngine<RNG,S>::MCPhoenixEngine(
             const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
             Size timeSteps,
             Size timeStepsPerYear,
             bool brownianBridge,
             bool antitheticVariate,
             Size requiredSamples,
             Real requiredTolerance,
             Size maxSamples,
             bool isBiased,
             BigNatural seed)
    : McSimulation<SingleVariate,RNG,S>(antitheticVariate, false),
      process_(process), timeSteps_(timeSteps),
      timeStepsPerYear_(timeStepsPerYear),
      requiredSamples_(requiredSamples), maxSamples_(maxSamples),
      requiredTolerance_(requiredTolerance),
      isBiased_(isBiased),
      brownianBridge_(brownianBridge), seed_(seed) {
        QL_REQUIRE(timeSteps != Null<Size>() ||
                   timeStepsPerYear != Null<Size>(),
                   "no time steps provided");
        QL_REQUIRE(timeSteps == Null<Size>() ||
                   timeStepsPerYear == Null<Size>(),
                   "both time steps and time steps per year were provided");
        QL_REQUIRE(timeSteps != 0,
                   "timeSteps must be positive, " << timeSteps <<
                   " not allowed");
        QL_REQUIRE(timeStepsPerYear != 0,
                   "timeStepsPerYear must be positive, " << timeStepsPerYear <<
                   " not allowed");
        registerWith(process_);
    }

	template <class RNG, class S>
    inline TimeGrid MCPhoenixEngine<RNG,S>::timeGrid() const {

        Time residualTime = process_->time(arguments_.exercise->lastDate());
        if (timeSteps_ != Null<Size>()) {
            return TimeGrid(residualTime, timeSteps_);
        } else if (timeStepsPerYear_ != Null<Size>()) {
            Size steps = static_cast<Size>(timeStepsPerYear_*residualTime);
            return TimeGrid(residualTime, std::max<Size>(steps, 1));
        } else {
            QL_FAIL("time steps not specified");
        }
    }


    template <class RNG, class S>
    inline
    ext::shared_ptr<typename MCPhoenixEngine<RNG,S>::path_pricer_type>
    MCPhoenixEngine<RNG,S>::pathPricer() const {
        ext::shared_ptr<PlainVanillaPayoff> payoff =
            ext::dynamic_pointer_cast<PlainVanillaPayoff>(arguments_.payoff);
        QL_REQUIRE(payoff, "non-plain payoff given");

        TimeGrid grid = timeGrid();
        std::vector<DiscountFactor> discounts(grid.size());
        for (Size i=0; i<grid.size(); i++)
            discounts[i] = process_->riskFreeRate()->discount(grid[i]);

        PseudoRandom::ursg_type sequenceGen(grid.size()-1,PseudoRandom::urng_type(5));
        return ext::shared_ptr< typename MCPhoenixEngine<RNG,S>::path_pricer_type >(
                new PhoenixPathPricer(
                    arguments_.principal_,
                    arguments_.upbarrier_,
					arguments_.downbarrier_,
                    arguments_.rebate_,
                    payoff->optionType(),
                    payoff->strike(),
                    discounts,
                    process_,
                    sequenceGen));
        }

    template <class RNG, class S>
    inline MakeMCPhoenixEngine<RNG,S>::MakeMCPhoenixEngine(
             const ext::shared_ptr<GeneralizedBlackScholesProcess>& process)
    : process_(process), brownianBridge_(false), antithetic_(false),
      biased_(false), steps_(Null<Size>()), stepsPerYear_(Null<Size>()),
      samples_(Null<Size>()), maxSamples_(Null<Size>()),
      tolerance_(Null<Real>()), seed_(0) {}

    template <class RNG, class S>
    inline MakeMCPhoenixEngine<RNG,S>&
    MakeMCPhoenixEngine<RNG,S>::withSteps(Size steps) {
        steps_ = steps;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCPhoenixEngine<RNG,S>&
    MakeMCPhoenixEngine<RNG,S>::withStepsPerYear(Size steps) {
        stepsPerYear_ = steps;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCPhoenixEngine<RNG,S>&
    MakeMCPhoenixEngine<RNG,S>::withBrownianBridge(bool brownianBridge) {
        brownianBridge_ = brownianBridge;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCPhoenixEngine<RNG,S>&
    MakeMCPhoenixEngine<RNG,S>::withAntitheticVariate(bool b) {
        antithetic_ = b;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCPhoenixEngine<RNG,S>&
    MakeMCPhoenixEngine<RNG,S>::withSamples(Size samples) {
        QL_REQUIRE(tolerance_ == Null<Real>(),
                   "tolerance already set");
        samples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCPhoenixEngine<RNG,S>&
    MakeMCPhoenixEngine<RNG,S>::withAbsoluteTolerance(Real tolerance) {
        QL_REQUIRE(samples_ == Null<Size>(),
                   "number of samples already set");
        QL_REQUIRE(RNG::allowsErrorEstimate,
                   "chosen random generator policy "
                   "does not allow an error estimate");
        tolerance_ = tolerance;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCPhoenixEngine<RNG,S>&
    MakeMCPhoenixEngine<RNG,S>::withMaxSamples(Size samples) {
        maxSamples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCPhoenixEngine<RNG,S>&
    MakeMCPhoenixEngine<RNG,S>::withBias(bool biased) {
        biased_ = biased;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCPhoenixEngine<RNG,S>&
    MakeMCPhoenixEngine<RNG,S>::withSeed(BigNatural seed) {
        seed_ = seed;
        return *this;
    }

    template <class RNG, class S>
    inline
    MakeMCPhoenixEngine<RNG,S>::operator ext::shared_ptr<PricingEngine>()
                                                                      const {
        QL_REQUIRE(steps_ != Null<Size>() || stepsPerYear_ != Null<Size>(),
                   "number of steps not given");
        QL_REQUIRE(steps_ == Null<Size>() || stepsPerYear_ == Null<Size>(),
                   "number of steps overspecified");
        return ext::shared_ptr<PricingEngine>(new
            MCPhoenixEngine<RNG,S>(process_,
                                   steps_,
                                   stepsPerYear_,
                                   brownianBridge_,
                                   antithetic_,
                                   samples_, tolerance_,
                                   maxSamples_,
                                   biased_,
                                   seed_));
    }

//}


#endif
