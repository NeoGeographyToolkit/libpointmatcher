// kate: replace-tabs off; indent-width 4; indent-mode normal
// vim: ts=4:sw=4:noexpandtab
/*

Copyright (c) 2010--2012,
Francois Pomerleau and Stephane Magnenat, ASL, ETHZ, Switzerland
You can contact the authors at <f dot pomerleau at gmail dot com> and
<stephane at magnenat dot net>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ETH-ASL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef __POINTMATCHER_INSPECTORS_H
#define __POINTMATCHER_INSPECTORS_H

#include "PointMatcher.h"
#include "Histogram.h"

template<typename T>
struct InspectorsImpl
{
	typedef PointMatcherSupport::Parametrizable Parametrizable;
	typedef PointMatcherSupport::Parametrizable P;
	typedef Parametrizable::Parameters Parameters;
	typedef Parametrizable::ParameterDoc ParameterDoc;
	typedef Parametrizable::ParametersDoc ParametersDoc;

	typedef typename PointMatcher<T>::Inspector Inspector;
	typedef typename PointMatcher<T>::DataPoints DataPoints;
	typedef typename PointMatcher<T>::Matches Matches;
	typedef typename PointMatcher<T>::OutlierWeights OutlierWeights;
	typedef typename PointMatcher<T>::TransformationParameters TransformationParameters;
	typedef typename PointMatcher<T>::TransformationCheckers TransformationCheckers;
	typedef typename PointMatcher<T>::Matrix Matrix;

	// Clearer name when no inspector is required
	struct NullInspector: public Inspector
	{
		inline static const std::string description()
		{
			return "Does nothing.";
		}
	};

	struct PerformanceInspector: public Inspector
	{
		inline static const std::string description()
		{
			return "Keep statistics on performance and dump iteration info.";
		}
		inline static const ParametersDoc availableParameters()
		{
			return {
				{ "baseFileName", "base file name for the statistics files (if empty, disabled)", "" },
				{ "dumpPerfOnExit", "dump performance statistics to stderr on exit", "0" },
				{ "dumpStats", "dump the statistics on first and last step", "0" },
				{ "dumpIterationInfo", "dump iteration info to CSV", "0" },
			};
		}

		const std::string baseFileName;
		const bool bDumpPerfOnExit;
		const bool bDumpStats;
		const bool bDumpIterationInfo;

	protected:
		typedef PointMatcherSupport::Histogram<double> Histogram;
		typedef std::map<std::string, Histogram> HistogramMap;
		HistogramMap stats;
		std::ostream* streamIter;

	public:
		PerformanceInspector(const std::string& className, const ParametersDoc paramsDoc, const Parameters& params);
		PerformanceInspector(const Parameters& params);

		virtual void init();
		virtual void addStat(const std::string& name, double data);
		virtual void dumpStats(std::ostream& stream);
		virtual void dumpStatsHeader(std::ostream& stream);
		virtual void dumpIteration(const size_t iterationNumber, const TransformationParameters& parameters, const DataPoints& filteredReference, const DataPoints& reading, const Matches& matches, const OutlierWeights& outlierWeights, const TransformationCheckers& transformationCheckers);
		virtual void finish(const size_t iterationCount);
	};
}; // InspectorsImpl

#endif // __POINTMATCHER_INSPECTORS_H
