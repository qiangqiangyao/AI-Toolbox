#ifndef AI_TOOLBOX_POMDP_MODEL_HEADER_FILE
#define AI_TOOLBOX_POMDP_MODEL_HEADER_FILE

#include <AIToolbox/Utils.hpp>
#include <AIToolbox/MDP/Types.hpp>
#include <AIToolbox/POMDP/Types.hpp>

#include <random>
#include <AIToolbox/Impl/Seeder.hpp>
#include <AIToolbox/ProbabilityUtils.hpp>

namespace AIToolbox {
    namespace POMDP {

#ifndef DOXYGEN_SKIP
        // This is done to avoid bringing around the enable_if everywhere.
        template <typename M, typename = typename std::enable_if<MDP::is_model<M>::value>::type>
        class Model;
#endif

        /**
         * @brief This class represents a Partially Observable Markov Decision Process.
         *
         * This class inherits from any valid MDP model type, so that it can
         * use its base methods, and it builds from those. Templated inheritance
         * was chosen to improve performance and keep code small, instead of
         * doing composition.
         *
         * @tparam M The particular MDP type that we want to extend.
         */
        template <typename M>
        class Model<M> : public M {
            public:
                using ObservationTable = Table3D;

                /**
                 * @brief Basic constructor.
                 *
                 * This constructor initializes the observation function
                 * so that all actions will return observation 0.
                 *
                 * @tparam Args All types of the parent constructor arguments.
                 * @param o The number of possible observations the agent could make.
                 * @param parameters All arguments needed to build the parent Model.
                 */
                template <typename... Args>
                Model(size_t o, Args&&... parameters);

                /**
                 * @brief Basic constructor.
                 *
                 * This constructor takes an arbitrary three dimensional
                 * containers and tries to copy its contents into the
                 * observations table.
                 *
                 * The container needs to support data access through
                 * operator[]. In addition, the dimensions of the
                 * container must match the ones provided as arguments
                 * both directly (o) and indirectly (s,a).
                 *
                 * This is important, as this constructor DOES NOT perform
                 * any size checks on the external containers.
                 *
                 * Internal values of the containers will be converted to double,
                 * so these convertions must be possible.
                 *
                 * In addition, the observation container must contain a
                 * valid transition function.
                 * \sa transitionCheck()
                 *
                 * \sa copyTable3D()
                 *
                 * @tparam ObFun The external observations container type.
                 * @param o The number of possible observations the agent could make.
                 * @param of The observation probability table.
                 * @param parameters All arguments needed to build the parent Model.
                 */
                template <typename ObFun, typename... Args>
                Model(size_t o, const ObFun & of, Args&&... parameters);

                /**
                 * @brief This function replaces the Model observation function with the one provided.
                 *
                 * The container needs to support data access through
                 * operator[]. In addition, the dimensions of the
                 * containers must match the ones provided as arguments
                 * (for three dimensions: s,a,o).
                 *
                 * This is important, as this constructor DOES NOT perform
                 * any size checks on the external containers.
                 *
                 * Internal values of the container will be converted to double,
                 * so that convertion must be possible.
                 *
                 * @tparam ObFun The external observations container type.
                 * @param table The external observations container.
                 */
                template <typename ObFun>
                void setObservationFunction(const ObFun & of);

                /**
                 * @brief This function returns the stored observation probability for the specified state-action pair.
                 *
                 * @param s1 The final state of the transition.
                 * @param a The action performed in the transition.
                 * @param o The recorded observation for the transition.
                 *
                 * @return The probability of the specified observation.
                 */
                double getObservationProbability(size_t s1, size_t a, size_t o) const;

                /**
                 * @brief This function *computes* the probability of obtaining an observation given an action and an initial belief.
                 *
                 * @param b The initial belief state.
                 * @param a The action performed.
                 * @param o The resulting observation.
                 *
                 * @return The probability of obtaining the specified observation.
                 */
                double getObservationProbability(const Belief & b, size_t o, size_t a) const;

                /**
                 * @brief Creates a new belief reflecting changes after an action and observation.
                 *
                 * This function creates a new belief since modifying a belief in place
                 * is not possible, as each cell update requires all values from the
                 * previous belief.
                 *
                 * @param b The old belief.
                 * @param a The action taken during the transition.
                 * @param o The observation registered.
                 */
                Belief updateBelief(const Belief & b, size_t a, size_t o) const;

                /**
                 * @brief This function returns the number of observations possible.
                 *
                 * @return The total number of observations.
                 */
                size_t getO() const;

                /**
                 * @brief This function returns the observation table for inspection.
                 *
                 * @return The rewards table.
                 */
                const ObservationTable & getObservationFunction() const;

            private:
                size_t O;
                ObservationTable observations_;
                // We need this because we don't know if our parent already has one,
                // and we wouldn't know how to access it!
                mutable std::default_random_engine rand_;
        };

        template <typename M>
        template <typename... Args>
        Model<M>::Model(size_t o, Args&&... params) : M(std::forward<Args>(params)...), O(o), observations_(boost::extents[this->getS()][this->getA()][O]) {
            for ( size_t s = 0; s < this->getS(); ++s )
                for ( size_t a = 0; a < this->getA(); ++a )
                    observations_[s][a][0] = 1.0;
        }

        template <typename M>
        template <typename ObFun, typename... Args>
        Model<M>::Model(size_t o, const ObFun & of, Args&&... params) : M(std::forward<Args>(params)...), O(o), observations_(boost::extents[this->getS()][this->getA()][O]),
                                                                        rand_(Impl::Seeder::getSeed())
        {
            setObservationFunction(of);
        }

        template <typename M>
        template <typename ObFun>
        void Model<M>::setObservationFunction(const ObFun & of) {
            for ( size_t s = 0; s < this->getS(); ++s )
                for ( size_t a = 0; a < this->getA(); ++a )
                    if ( ! isProbability(of[s][a], O) ) throw std::invalid_argument("Input observation table does not contain valid probabilities.");

            copyTable3D(of, observations_, this->getS(), this->getA(), O);
        }

        template <typename M>
        Belief Model<M>::updateBelief(const Belief & b, size_t a, size_t o) const {
            size_t S = this->getS();
            Belief br(S, 0.0);

            for ( size_t s1 = 0; s1 < S; ++s1 ) {
                double sum = 0.0;
                for ( size_t s = 0; s < S; ++s )
                    sum += this->getTransitionProbability(s,a,s1) * b[s];

                br[s1] = getObservationProbability(s1,a,o) * sum;
            }

            return br;
        }

        template <typename M>
        double Model<M>::getObservationProbability(size_t s1, size_t a, size_t o) const {
            return observations_[s1][a][o];
        }

        template <typename M>
        size_t Model<M>::getO() const {
            return O;
        }

        template <typename M>
        const typename Model<M>::ObservationTable & Model<M>::getObservationFunction() const {
            return observations_;
        }
    }
}

#endif
