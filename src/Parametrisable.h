/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#ifndef DATA_PARAMETRISABLE_H
#define DATA_PARAMETRISABLE_H

#include "Parameter.h"

#include <vector>
#include <cassert>

namespace Data {

    /**
     * Convenience class for the architectural components that are
     * parametrisable. The interface is shared and implemented only in
     * this class.
     */
    class Parametrisable {
    public:

        Parametrisable() {
        };

        virtual ~Parametrisable() {
        };

        /**
         * Push a new parameter into the in-order vector without checking
         * for duplicates.
         */
        virtual void pushParameter(const Parameter& parameter);

        /**
         * Set a parameter with a given index (default 0). This could for
         * example be a queue size for a given channel id.
         */
        void setParameter(const Parameter& parameter, unsigned int index = 0);

        /**
         * Get a parameter of a given name and of a certain index. Calling
         * this method on an object that has no parameter of that name
         * will result in application exit.
         */
        Parameter getParameter(const std::string& id,
                unsigned int index = 0) const;

        /**
         * Remove a parameter with a specific name and index. If a parameter
         * is removed this method returns true, otherwise false.
         */
        bool removeParameter(const std::string& id, unsigned int index = 0);

        /**
         * Simply get all the parameters.
         */
        std::vector<Parameter> getParameters() const;

        /**
         * Check if a parameter of a certain name exists in the object.
         */
        bool hasParameter(const std::string& id, unsigned int index = 0) const;

        /**
         * Convenience function to set a variable to the value stored in a parameter
         * with built in error detection/assertion. Returns true if the value was
         * successfully set.
         * @param m
         * @param paramName
         * @param assertOnFail
         * @return
         */
        template<typename T>
        bool setVarFromParam(T* m, const char* paramName) {
            if (hasParameter(paramName)) {
                *m = static_cast<T> (getParameter(paramName));
                return true;
            } else {
                *m = static_cast<T> (0);
                return false;
            }
        }

        template<typename T>
        T getParamValWithDefault(const char* paramName, T defaultVal) const {
            if (hasParameter(paramName)) {
            return static_cast<T>(getParameter(paramName));
            }
            else {
            return defaultVal;
            }
        }

    protected:

        std::vector<Parameter> parameters;
    };
}
#endif
