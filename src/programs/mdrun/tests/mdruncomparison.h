/*
 * This file is part of the GROMACS molecular simulation package.
 *
 * Copyright (c) 2016,2018, by the GROMACS development team, led by
 * Mark Abraham, David van der Spoel, Berk Hess, and Erik Lindahl,
 * and including many others, as listed in the AUTHORS file in the
 * top-level source directory and at http://www.gromacs.org.
 *
 * GROMACS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * GROMACS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GROMACS; if not, see
 * http://www.gnu.org/licenses, or write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 *
 * If you want to redistribute modifications to GROMACS, please
 * consider that scientific software is very special. Version
 * control is crucial - bugs must be traceable. We will be happy to
 * consider code for inclusion in the official distribution, but
 * derived work must not be called official GROMACS. Details are found
 * in the README & COPYING files - if they are missing, get the
 * official version at http://www.gromacs.org.
 *
 * To help us fund GROMACS development, we humbly ask that you cite
 * the research papers on the package. Check out http://www.gromacs.org.
 */
/*! \libinternal
 *
 * \brief Functionality for testing whether calls to mdrun produce the
 * same energy and force quantities when they should do so.
 */
#ifndef GMX_MDRUN_TESTS_MDRUNCOMPARISON_H
#define GMX_MDRUN_TESTS_MDRUNCOMPARISON_H

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "testutils/cmdlinetest.h"

#include "moduletest.h"

namespace gmx
{

namespace test
{

//! Helper typedef
using MdpFieldValues = std::map<std::string, std::string>;

/*! \brief Set up values for an .mdp file that permits a highly
 * reproducible simulation.
 *
 * An internal database of several kinds of simulation useful for such
 * comparisons is available, whose \c simulationName keys are
 *     - argon12
 *     - argon5832
 *     - spc5
 *     - spc216
 *     - alanine_vsite_vacuo
 *     - alanine_vsite_solvated
 *     - nonanol
 *
 * Some of these systems are pretty minimal, because having
 * few atoms means few interactions, highly reproducible
 * forces, and allows tests to focus on the correctness of the
 * implementation of high-level mdrun features. The boxes are
 * of a reasonable size so that domain decomposition is
 * possible. The pressure-coupling parameters are isotropic,
 * and set up so that there will not be dramatic collapse of
 * volume over the handful of MD steps that will be run. A
 * single temperature-coupling group is used.
 *
 * \param[in]    simulationName   The name of the simulation, which indexes the database
 * \param[in]    integrator       The integrator to use
 * \param[in]    tcoupl           The temperature-coupling algorithm to use
 * \param[in]    pcoupl           The pressure-coupling algorithm to use
 * \return                        Mdp file values
 *
 * \throws  std::bad_alloc     if out of memory
 *          std::out_of_range  if \c simulationName is not in the database */
MdpFieldValues
prepareMdpFieldValues(const char *simulationName,
                      const char *integrator,
                      const char *tcoupl,
                      const char *pcoupl);

/*! \brief Make a string containing an .mdp file from the \c mdpFieldValues.
 *
 * \throws  std::bad_alloc     if out of memory */
std::string
prepareMdpFileContents(const MdpFieldValues &mdpFieldValues);

/*! \internal
 * \brief Manages returning a pair of frames from two
 * equivalent simulations that are meaningful to compare. */
template <class FrameReader, class Frame>
class FramePairManager
{
    public:
        //! Convenience typedef
        typedef std::unique_ptr<FrameReader> FrameReaderPtr;
        //! Constructor
        FramePairManager(FrameReaderPtr first,
                         FrameReaderPtr second) :
            first_(std::move(first)),
            second_(std::move(second))
        {}
    private:
        /*! \brief Probe for a pair of valid frames, and return true if both are found.
         *
         * Give a test failure if exactly one frame is found, because
         * that file is longer than the other one, and this is not
         * expected behaviour. */
        bool shouldContinueComparing()
        {
            if (first_->readNextFrame())
            {
                if (second_->readNextFrame())
                {
                    // Two valid next frames exist, so we should continue comparing.
                    return true;
                }
                else
                {
                    ADD_FAILURE() << "first file had at least one more frame than second file";
                }
            }
            else
            {
                if (second_->readNextFrame())
                {
                    ADD_FAILURE() << "second file had at least one more frame than first file";
                }
                else
                {
                    // Both files ran out of frames at the same time, which is the expected behaviour.
                }
            }
            // At least one file is out of frames, so should not continue comparing.
            return false;
        }
    public:
        //! Compare all possible pairs of frames using \c compareTwoFrames.
        void compareAllFramePairs(std::function<void(const Frame &, const Frame &)> compareTwoFrames)
        {
            while (shouldContinueComparing())
            {
                auto firstFrame  = first_->frame();
                auto secondFrame = second_->frame();
                SCOPED_TRACE("Comparing frames from two runs '" + firstFrame.frameName() + "' and '" + secondFrame.frameName() + "'");
                compareTwoFrames(firstFrame, secondFrame);
            }

        }
    private:
        FrameReaderPtr first_;
        FrameReaderPtr second_;
};

} // namespace test
} // namespace gmx

#endif
