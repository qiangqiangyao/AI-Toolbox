#ifndef AI_TOOLBOX_LP_HEADER_FILE
#define AI_TOOLBOX_LP_HEADER_FILE

#include <memory>
#include <optional>

#include <AIToolbox/Types.hpp>

namespace AIToolbox {
    /**
     * @brief This class presents a common interface for solving Linear Programming problems.
     *
     * This class presents a library-agnostic interface to solve Linear
     * Programming problems. Ideally this class can be implemented through
     * different libraries as needed, in order to avoid changing the rest of
     * the library.
     *
     * Constraints are added through editing the `row` public Vector. This
     * vector has a number of elements equal to the number of variables
     * specified to the LP class during construction. Each element in the
     * Vector corresponds to the coefficient of the associated variable.
     */
    class LP {
        private:
            // Data concerning any specific library we use to solve LPs
            struct LP_impl;
            std::unique_ptr<LP_impl> pimpl_;

        public:
            enum class Constraint { LessEqual, Equal, GreaterEqual };
            /**
             * @brief Basic constructor.
             *
             * With this constructor one must specify the number of variables
             * (columns) of the underlying LP problem.
             *
             * By default all variables are assumed positive (>=0).
             *
             * @param varNumber The number of variables.
             */
            LP(size_t varNumber);

            /**
             * @brief Basic destructor to avoid problems with std::unique_ptr
             */
            ~LP();

            /**
             * @brief Editable vector containing column coefficients.
             *
             * This field will NEVER be touched by this class, so you are free
             * to set it as you please, and its value will not be modified.
             */
            Eigen::Map<Vector> row;

            /**
             * @brief This function selects the variable to use as objective.
             *
             * In addition it allows to specify whether that variable should be
             * maximized or minimized.
             *
             * @param n The id of the variable to select as objective.
             * @param maximize Whether the variable should be maximized (or minimized).
             */
            void setObjective(size_t n, bool maximize);

            /**
             * @brief This function adds a constraint to the LP.
             *
             * This function adds the current contents of the public field
             * `row` as a constraints. The `row` field remains untouched.
             *
             * Rows are treated as a stack, and thus pushes the new row at the
             * top of the stack.
             *
             * @param c The type of constraint that should be enforced.
             * @param value The value on the other side of the constraint equation.
             */
            void pushRow(Constraint c, double value);

            /**
             * @brief This function removes the last pushed constraint.
             */
            void popRow();

            /**
             * @brief This function solves the LP associated with all constraints in the stack.
             *
             * This function solves the currently set LP problem. If solved,
             * the return Vector will contain the values of the first N
             * variables of the solution, where N is the input.
             *
             * @param variables The number of variables one wants the solution of.
             *
             * @return A Vector if the solving process succeeded.
             */
            std::optional<Vector> solve(size_t variables);

            /**
             * @brief This function resizes the underlying LP.
             *
             * This function can be used to both pre-allocate memory in advance
             * before pushing many rows, and to rapidly prune already set rows.
             *
             * The number passed represents the number of rows that one wants
             * to leave to the LP. If the number is higher than the number of
             * currently set rows, this function may allocate memory to reserve
             * at least the input of rows.
             *
             * If the number is lower, the effect is equivalent to calling
             * popRow() until the remaining number of rules is equal to the
             * number specified.
             *
             * @param rows
             */
            void resize(size_t rows);
            /**
             * @brief This function sets the specified variable as unbounded.
             *
             * Normally all variables are assumed positive (>=0). This function
             * is needed in case a variable needs to be unbounded.
             *
             * @param n The variable to make unbounded.
             */
            void setUnbounded(size_t n);

        private:
            bool maximize_;
    };
}

#endif
