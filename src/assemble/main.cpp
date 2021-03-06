//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#include <iostream>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <execinfo.h>

#include "../sequence/vertex_index.h"
#include "../sequence/sequence_container.h"
#include "../sequence/overlap.h"
#include "../sequence/consensus_generator.h"
#include "../common/config.h"
#include "chimera.h"
#include "extender.h"
#include "parameters_estimator.h"
#include "../common/logger.h"

bool parseArgs(int argc, char** argv, std::string& readsFasta, 
			   std::string& outAssembly, std::string& logFile, int& coverage,
			   int& kmerSize, int& minKmer, int& maxKmer, bool& debug,
			   size_t& numThreads, std::string& overlapsFile, int& minOverlap)
{
	auto printUsage = [argv]()
	{
		std::cerr << "Usage: " << argv[0]
				  << "\treads_file out_assembly coverage \n\t\t\t\t"
				  << "[-k kmer_size] [-m min_kmer_cov] \n\t\t\t\t"
				  << "[-x max_kmer_cov] [-l log_file] [-t num_threads] [-d]\n\n"
				  << "positional arguments:\n"
				  << "\treads file\tpath to fasta with reads\n"
				  << "\tout_assembly\tpath to output file\n"
				  << "\tcoverage\testimated assembly coverage\n"
				  << "\noptional arguments:\n"
				  << "\t-k kmer_size\tk-mer size [default = 15] \n"
				  << "\t-m min_kmer_cov\tminimum k-mer coverage "
				  << "[default = auto] \n"
				  << "\t-x max_kmer_cov\tmaximum k-mer coverage "
				  << "[default = auto] \n"
				  << "\t-v min_overlap\tminimum overlap between reads "
				  << "[default = 5000] \n"
				  << "\t-d \t\tenable debug output "
				  << "[default = false] \n"
				  << "\t-l log_file\toutput log to file "
				  << "[default = not set] \n"
				  << "\t-o ovlp_file\tstore/load overlaps to/from file "
				  << "[default = not set] \n"
				  << "\t-t num_threads\tnumber of parallel threads "
				  << "[default = 1] \n";
	};

	kmerSize = 15;
	minKmer = -1;
	maxKmer = -1;
	numThreads = 1;
	debug = false;
	logFile = "";
	overlapsFile = "";
	minOverlap = 5000;

	const char optString[] = "k:m:x:l:t:o:v:hd";
	int opt = 0;
	while ((opt = getopt(argc, argv, optString)) != -1)
	{
		switch(opt)
		{
		case 'k':
			kmerSize = atoi(optarg);
			break;
		case 'm':
			minKmer = atoi(optarg);
			break;
		case 'x':
			maxKmer = atoi(optarg);
			break;
		case 't':
			numThreads = atoi(optarg);
			break;
		case 'v':
			minOverlap = atoi(optarg);
			break;
		case 'l':
			logFile = optarg;
			break;
		case 'o':
			overlapsFile = optarg;
			break;
		case 'd':
			debug = true;
			break;
		case 'h':
			printUsage();
			exit(0);
		}
	}
	if (argc - optind != 3)
	{
		printUsage();
		return false;
	}
	readsFasta = *(argv + optind);
	outAssembly = *(argv + optind + 1);
	coverage = atoi(*(argv + optind + 2));

	return true;
}

bool fileExists(const std::string& path)
{
	std::ifstream fin(path);
	return fin.good();
}

void segfaultHandler(int signal)
{
	void *stackArray[20];
	size_t size = backtrace(stackArray, 10);
	Logger::get().error() << "Segmentation fault! Backtrace:";
	char** backtrace = backtrace_symbols(stackArray, size);
	for (size_t i = 0; i < size; ++i)
	{
		Logger::get().error() << "\t" << backtrace[i];
	}
	exit(1);
}

void exceptionHandler()
{
	static bool triedThrow = false;
	try
	{
        if (!triedThrow)
		{
			triedThrow = true;
			throw;
		}
    }
    catch (const std::exception &e) 
	{
        Logger::get().error() << "Caught unhandled exception: " << e.what();
    }
	catch (...) {}

	void *stackArray[20];
	size_t size = backtrace(stackArray, 10);
	char** backtrace = backtrace_symbols(stackArray, size);
	for (size_t i = 0; i < size; ++i)
	{
		Logger::get().error() << "\t" << backtrace[i];
	}
	exit(1);
}

int main(int argc, char** argv)
{
	#ifndef _DEBUG
	signal(SIGSEGV, segfaultHandler);
	std::set_terminate(exceptionHandler);
	#endif

	int kmerSize = 0;
	int minKmerCov = 0;
	int maxKmerCov = 0;
	int coverage = 0;
	int minOverlap = 0;
	bool debugging = false;
	size_t numThreads;
	std::string readsFasta;
	std::string outAssembly;
	std::string logFile;
	std::string overlapsFile;

	if (!parseArgs(argc, argv, readsFasta, outAssembly, logFile, coverage,
				   kmerSize, minKmerCov, maxKmerCov, debugging, numThreads,
				   overlapsFile, minOverlap)) 
	{
		return 1;
	}
	Parameters::get().minimumOverlap = minOverlap;
	Parameters::get().kmerSize = kmerSize;
	Parameters::get().numThreads = numThreads;

	Logger::get().setDebugging(debugging);
	if (!logFile.empty()) Logger::get().setOutputFile(logFile);
	std::ios::sync_with_stdio(false);

	SequenceContainer readsContainer;
	Logger::get().debug() << "Build date: " << __DATE__ << " " << __TIME__;
	Logger::get().info() << "Reading sequences";
	try
	{
		readsContainer.loadFromFile(readsFasta);
	}
	catch (SequenceContainer::ParseException& e)
	{
		Logger::get().error() << e.what();
		return 1;
	}
	VertexIndex vertexIndex(readsContainer);
	vertexIndex.outputProgress(true);

	int64_t sumLength = 0;
	for (auto& seqId : readsContainer.getIndex())
	{
		sumLength += seqId.second.sequence.length();
	}
	Logger::get().debug() << "Mean read length: " 
		<< sumLength / readsContainer.getIndex().size();

	//rough estimate
	Logger::get().info() << "Generating solid k-mer index";
	size_t hardThreshold = std::max(1, coverage / 
									Constants::hardMinCoverageRate);
	vertexIndex.countKmers(hardThreshold);

	if (maxKmerCov == -1)
	{
		maxKmerCov = Constants::repeatCoverageRate * coverage;
	}

	ParametersEstimator estimator(readsContainer, vertexIndex, coverage);
	estimator.estimateMinKmerCount(maxKmerCov);
	if (minKmerCov == -1)
	{
		minKmerCov = estimator.minKmerCount();
	}


	vertexIndex.buildIndex(minKmerCov, maxKmerCov, 
						   Constants::assembleKmerSample);

	OverlapDetector ovlp(readsContainer, vertexIndex,
						 Constants::maximumJump, 
						 Parameters::get().minimumOverlap,
						 Constants::maximumOverhang, 
						 Constants::assembleGap,
						 /*store alignment*/ false);
	OverlapContainer readOverlaps(ovlp, readsContainer, /*only max*/ true);

	Extender extender(readsContainer, readOverlaps, coverage, 
					  estimator.genomeSizeEstimate());
	extender.assembleContigs();
	vertexIndex.clear();

	ConsensusGenerator consGen;
	auto contigsFasta = consGen.generateConsensuses(extender.getContigPaths());
	SequenceContainer::writeFasta(contigsFasta, outAssembly);

	return 0;
}
