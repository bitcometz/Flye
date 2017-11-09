//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#include "parameters_estimator.h"
#include "../common/logger.h"


int ParametersEstimator::genomeSizeEstimate()
{
	/*
	int kmersNeeded = 0;
	for (auto& seqPair : _seqContainer.getIndex()) 
	{
		kmersNeeded += _seqContainer.seqLen(seqPair.first) / _coverage;
	}
	return kmersNeeded / 2;*/
	return _takenKmers;
}


void ParametersEstimator::estimateMinKmerCount(int upperCutoff)
{
	const int MIN_CUTOFF = 2;

	int kmersNeeded = 0;
	for (auto& seqPair : _seqContainer.getIndex()) 
	{
		kmersNeeded += _seqContainer.seqLen(seqPair.first) / 2 / _coverage;
	}
	Logger::get().debug() << "Genome size estimate: " << kmersNeeded;
	
	int takenKmers = 0;
	int cutoff = 0;
	int repetitiveKmers = 0;
	int prevDiff = 0;
	for (auto mapPair = _vertexIndex.getKmerHist().rbegin();
		 mapPair != _vertexIndex.getKmerHist().rend(); ++mapPair)
	{
		if (mapPair->first <= upperCutoff)
		{
			takenKmers += mapPair->second;
			if (takenKmers >= kmersNeeded)
			{
				if (abs(takenKmers - kmersNeeded) < prevDiff)
				{
					cutoff = mapPair->first;
				}
				else
				{
					cutoff = mapPair->first + 1;
					takenKmers -= mapPair->second;
				}
				break;
			}
			prevDiff = abs(takenKmers - kmersNeeded);
		}
		else
		{
			repetitiveKmers += mapPair->second;
		}
	}

	if (cutoff < 2)
	{
		Logger::get().warning() << "Unable to separate erroneous k-mers "
					  "from solid k-mers. Possible reasons: \n"
					  "\t(1) Incorrect expected assembly size parameter \n"
					  "\t(2) Highly uneven coverage of the assembly \n"
					  "\t(3) Running with error-corrected reads "
					  "(please use raw reads instead)\n"
					  "\t(4) Assembling big genome with small k-mer size "
					  "(try to increase it manually).\n"
					  "\tAssembly will continue, but results might not be optimal";
		cutoff = MIN_CUTOFF;
	}
	
	Logger::get().debug() << "Filtered " << repetitiveKmers 
						  << " repetitive kmers";
	Logger::get().debug() << "Estimated minimum kmer coverage: " << cutoff 
						  << ", " << takenKmers << " unique kmers selected";

	_takenKmers = takenKmers;
	_minKmerCount = cutoff;
}
