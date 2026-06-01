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

#include "InspectorsImpl.h"

#include "PointMatcherPrivate.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;
using namespace PointMatcherSupport;

template<typename T>
InspectorsImpl<T>::PerformanceInspector::PerformanceInspector(const std::string& className, const ParametersDoc paramsDoc, const Parameters& params):
	Inspector(className,paramsDoc,params),
	baseFileName(Parametrizable::get<string>("baseFileName")),
	bDumpPerfOnExit(Parametrizable::get<bool>("dumpPerfOnExit")),
	bDumpStats(Parametrizable::get<bool>("dumpStats")),
	bDumpIterationInfo(Parametrizable::get<bool>("dumpIterationInfo")),
	streamIter(nullptr)
{}

template<typename T>
InspectorsImpl<T>::PerformanceInspector::PerformanceInspector(const Parameters& params):
	Inspector("PerformanceInspector", PerformanceInspector::availableParameters(), params),
	baseFileName(Parametrizable::get<string>("baseFileName")),
	bDumpPerfOnExit(Parametrizable::get<bool>("dumpPerfOnExit")),
	bDumpStats(Parametrizable::get<bool>("dumpStats")),
	bDumpIterationInfo(Parametrizable::get<bool>("dumpIterationInfo")),
	streamIter(nullptr)
{}

template<typename T>
void InspectorsImpl<T>::PerformanceInspector::init()
{
	if (!bDumpIterationInfo || baseFileName.empty())
		return;

	ostringstream oss;
	oss << baseFileName << "-iterationInfo.csv";

	streamIter = new ofstream(oss.str().c_str());
	if (streamIter->fail())
		throw std::runtime_error("Couldn't open the file \"" + oss.str() + "\".");
}

template<typename T>
void InspectorsImpl<T>::PerformanceInspector::addStat(const std::string& name, double data)
{
	if (!bDumpStats) return;

	HistogramMap::iterator it(stats.find(name));
	if (it == stats.end()) {
		LOG_INFO_STREAM("Adding new stat: " << name);
		it = stats.insert(HistogramMap::value_type(name, Histogram(16, name, baseFileName, bDumpPerfOnExit))).first;
	}
	it->second.push_back(data);
}

template<typename T>
void InspectorsImpl<T>::PerformanceInspector::dumpStats(std::ostream& stream)
{
	for (auto it = stats.begin(); it != stats.end(); ++it)
	{
		it->second.dumpStats(stream);
		auto jt = it;
		++jt;
		if (jt != stats.end())
			stream << ", ";
	}
}

template<typename T>
void InspectorsImpl<T>::PerformanceInspector::dumpStatsHeader(std::ostream& stream)
{
	for (auto it = stats.begin(); it != stats.end(); ++it)
	{
		it->second.dumpStatsHeader(stream);
		auto jt = it;
		++jt;
		if (jt != stats.end())
			stream << ", ";
	}
}

template<typename T>
void InspectorsImpl<T>::PerformanceInspector::dumpIteration(
	const size_t iterationNumber,
	const TransformationParameters& parameters,
	const DataPoints& filteredReference,
	const DataPoints& reading,
	const Matches& matches,
	const OutlierWeights& outlierWeights,
	const TransformationCheckers& transCheck)
{
	if (!bDumpIterationInfo || !streamIter)
		return;

	if (iterationNumber == 0)
	{
		// Build header
		for (unsigned int j = 0; j < transCheck.size(); j++)
		{
			for (unsigned int i = 0; i < transCheck[j]->getConditionVariableNames().size(); i++)
			{
				if (!(j == 0 && i == 0))
					*streamIter << ", ";
				*streamIter << transCheck[j]->getConditionVariableNames()[i] << ", ";
				*streamIter << transCheck[j]->getLimitNames()[i];
			}
		}

		*streamIter << "\n";
	}

	for (unsigned int j = 0; j < transCheck.size(); j++)
	{
		for (unsigned int i = 0; i < transCheck[j]->getConditionVariables().size(); i++)
		{
			if (!(j == 0 && i == 0))
				*streamIter << ", ";

			*streamIter << transCheck[j]->getConditionVariables()[i] << ", ";
			*streamIter << transCheck[j]->getLimits()[i];
		}
	}

	*streamIter << "\n";
}

template<typename T>
void InspectorsImpl<T>::PerformanceInspector::finish(const size_t iterationCount)
{
	if (streamIter)
	{
		delete streamIter;
		streamIter = nullptr;
	}
}

template struct InspectorsImpl<float>::PerformanceInspector;
template struct InspectorsImpl<double>::PerformanceInspector;
