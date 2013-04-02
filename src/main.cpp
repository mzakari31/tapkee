/* This software is distributed under BSD 3-clause license (see LICENSE file).
 *
 * Copyright (c) 2012-2013 Sergey Lisitsyn, Fernando Iglesias 
 */

#include <tapkee.hpp>
#include <tapkee_defines.hpp>
#include <tapkee_projection.hpp>
#include <callback/eigen_callbacks.hpp>
#include <callback/precomputed_callbacks.hpp>
#include <utils/logging.hpp>
#include <algorithm>
#include <string>
#include <vector>
#include <iterator>
#include "ezoptionparser.hpp"
#include "util.hpp"

#ifdef GIT_INFO
	#define TAPKEE_CURRENT_GIT_INFO GIT_INFO
#else
	#define TAPKEE_CURRENT_GIT_INFO "unknown"
#endif

using namespace ez;
using namespace Eigen;
using namespace std;

bool cancel()
{
	return false;
}

int run(int argc, const char** argv)
{
	srand(static_cast<unsigned int>(time(NULL)));

	ezOptionParser opt;
	opt.footer = "Copyright (C) 2012-2013 Sergey Lisitsyn <lisitsyn.s.o@gmail.com>, Fernando Iglesias <fernando.iglesiasg@gmail.com>\n"
	             "This is free software: you are free to change and redistribute it.\n"
	             "There is NO WARRANTY, to the extent permitted by law.";
	opt.overview = "Tapkee library application for reduction dimensions of dense matrices.\n"
	               "Git " TAPKEE_CURRENT_GIT_INFO;
	opt.example = "Run locally linear embedding with k=10 with arpack "
                  "eigensolver on data from input.dat saving embedding to output.dat \n\n"
	              "tapkee -i input.dat -o output.dat --method lle --eigen-method arpack -k 10\n\n";
	opt.syntax = "tapkee [options]\n";

#if defined(_WIN32) || defined(_WIN64)
	#define OPT_PREFIX "/"
	#define OPT_LONG_PREFIX "/"
#else
	#define OPT_PREFIX "-"
	#define OPT_LONG_PREFIX "--"
#endif

#define INPUT_FILE_KEYWORD "input-file"
	opt.add("",0,1,0,"Input file",
			OPT_PREFIX "i",
		    OPT_LONG_PREFIX INPUT_FILE_KEYWORD);
#define TRANSPOSE_INPUT_KEYWORD "transpose"
	opt.add("",0,0,0,"Transpose input file if set",
		OPT_LONG_PREFIX TRANSPOSE_INPUT_KEYWORD);
#define OUTPUT_FILE_KEYWORD "output-file"
	opt.add("",0,1,0,"Output file",
		OPT_PREFIX "o",
		OPT_LONG_PREFIX OUTPUT_FILE_KEYWORD);
#define OUTPUT_PROJECTION_MATRIX_FILE_KEYWORD "output-projection-matrix-file"
	opt.add("",0,1,0,"Output file for projection matrix",
		OPT_PREFIX "opmat",
		OPT_LONG_PREFIX OUTPUT_PROJECTION_MATRIX_FILE_KEYWORD);
#define OUTPUT_PROJECTION_MEAN_FILE_KEYWORD "output-projection-mean-file"
	opt.add("",0,1,0,"Output file for mean of data",
		OPT_PREFIX "opmean",
		OPT_LONG_PREFIX OUTPUT_PROJECTION_MEAN_FILE_KEYWORD);
#define HELP_KEYWORD "help"
	opt.add("",0,0,0,"Display help",
		OPT_PREFIX "h",
		OPT_LONG_PREFIX HELP_KEYWORD);
#define BENCHMARK_KEYWORD "benchmark"
	opt.add("",0,0,0,"Output benchmark information",
		OPT_LONG_PREFIX BENCHMARK_KEYWORD);
#define VERBOSE_KEYWORD "verbose"
	opt.add("",0,0,0,"Output more information",
		OPT_LONG_PREFIX VERBOSE_KEYWORD);
#define DEBUG_KEYWORD "debug"
	opt.add("",0,0,0,"Output debug information",
		OPT_LONG_PREFIX DEBUG_KEYWORD);
#define METHOD_KEYWORD "method"
	opt.add("locally_linear_embedding",0,1,0,
			"Dimension reduction method (default locally_linear_embedding). \n One of the following: \n"
			"locally_linear_embedding (lle), neighborhood_preserving_embedding (npe), \n"
			"local_tangent_space_alignment (ltsa), linear_local_tangent_space_alignment (lltsa), \n"
			"hessian_locally_linear_embedding (hlle), laplacian_eigenmaps (la), locality_preserving_projections (lpp), \n"
			"diffusion_map (dm), isomap, landmark_isomap (l-isomap), multidimensional_scaling (mds), \n"
			"landmark_multidimensional_scaling (l-mds), stochastic_proximity_embedding (spe), \n"
			"kernel_pca (kpca), pca, random_projection (ra), factor_analysis (fa), t-stochastic_neighborhood_embedding (t-sne).",
			OPT_PREFIX "m",
			OPT_LONG_PREFIX METHOD_KEYWORD);
#define NEIGHBORS_METHOD_KEYWORD "neighbors-method"
	opt.add(
#ifdef TAPKEE_USE_LGPL_COVERTREE
			"covertree",
#else
			"brute",
#endif
			0,1,0,"Neighbors search method (default is 'covertree' if available, 'brute' otherwise). One of the following: "
			"brute"
#ifdef TAPKEE_USE_LGPL_COVERTREE
			",covertree"
#endif
			".",
			OPT_PREFIX "nm",
			OPT_LONG_PREFIX NEIGHBORS_METHOD_KEYWORD);
#define EIGEN_METHOD_KEYWORD "eigen-method"
	opt.add(
#ifdef TAPKEE_WITH_ARPACK
		"arpack",
#else
		"dense",
#endif 
		0,1,0,"Eigendecomposition method (default is 'arpack' if available, 'dense' otherwise). One of the following: "
#ifdef TAPKEE_WITH_ARPACK	
		"arpack, "
#endif
		"randomized, dense.",
		OPT_PREFIX "em",
		OPT_LONG_PREFIX EIGEN_METHOD_KEYWORD);
#define TARGET_DIMENSION_KEYWORD "target-dimension"
	opt.add("2",0,1,0,"Target dimension (default 2)",
		OPT_PREFIX "td",
		OPT_LONG_PREFIX TARGET_DIMENSION_KEYWORD);
#define NUM_NEIGHBORS_KEYWORD "num-neighbors"
	opt.add("10",0,1,0,"Number of neighbors (default 10)",
		OPT_PREFIX "k",
		OPT_LONG_PREFIX NUM_NEIGHBORS_KEYWORD);
#define GAUSSIAN_WIDTH_KEYWORD "gaussian-width"
	opt.add("1.0",0,1,0,"Width of gaussian kernel (default 1.0)",
		OPT_PREFIX "gw",
		OPT_LONG_PREFIX GAUSSIAN_WIDTH_KEYWORD);
#define TIMESTEPS_KEYWORD "timesteps"
	opt.add("1",0,1,0,"Number of timesteps for diffusion map (default 1)",
		OPT_LONG_PREFIX TIMESTEPS_KEYWORD);
#define SPE_LOCAL_KEYWORD "spe-local"
	opt.add("0",0,0,0,"Local strategy in SPE (default global)",
		OPT_LONG_PREFIX SPE_LOCAL_KEYWORD);
#define EIGENSHIFT_KEYWORD "eigenshift"
	opt.add("1e-9",0,1,0,"Regularization diagonal shift for weight matrix (default 1e-9)",
		OPT_LONG_PREFIX EIGENSHIFT_KEYWORD);
#define LANDMARK_RATIO_KEYWORD "landmark-ratio"
	opt.add("0.2",0,1,0,"Ratio of landmarks. Should be in (0,1) range (default 0.2, i.e. 20%)",
		OPT_LONG_PREFIX LANDMARK_RATIO_KEYWORD);
#define SPE_TOLERANCE_KEYWORD "spe-tolerance"
	opt.add("1e-5",0,1,0,"Tolerance for SPE (default 1e-5)",
		OPT_LONG_PREFIX SPE_TOLERANCE_KEYWORD);
#define SPE_NUM_UPDATES_KEYWORD "spe-num-updates"
	opt.add("100",0,1,0,"Number of SPE updates (default 100)",
		OPT_LONG_PREFIX SPE_NUM_UPDATES_KEYWORD);
#define MAX_ITERS_KEYWORD "max-iters"
	opt.add("1000",0,1,0,"Maximum number of iterations (default 1000)",
		OPT_LONG_PREFIX MAX_ITERS_KEYWORD);
#define FA_EPSILON_KEYWORD "fa-epsilon"
	opt.add("1e-5",0,1,0,"FA convergence criterion (default 1e-5)",
		OPT_LONG_PREFIX FA_EPSILON_KEYWORD);
#define SNE_PERPLEXITY_KEYWORD "sne-perplexity"
	opt.add("30.0",0,1,0,"Perplexity for the t-SNE algorithm (default 30.0)",
		OPT_LONG_PREFIX SNE_PERPLEXITY_KEYWORD);
#define SNE_THETA_KEYWORD "sne-theta"
	opt.add("0.5",0,1,0,"Theta for the t-SNE algorithm (defualt 0.5)",
		OPT_LONG_PREFIX SNE_THETA_KEYWORD);

	opt.parse(argc, argv);

	if (opt.isSet(OPT_LONG_PREFIX HELP_KEYWORD))
	{
		string usage;
		opt.getUsage(usage);
		std::cout << usage << std::endl;
		return 0;
	}

	if (opt.isSet(OPT_LONG_PREFIX VERBOSE_KEYWORD))
	{
		tapkee::LoggingSingleton::instance().enable_info();
	}
	if (opt.isSet(OPT_LONG_PREFIX DEBUG_KEYWORD))
	{
		tapkee::LoggingSingleton::instance().enable_debug();
		tapkee::LoggingSingleton::instance().message_info("Debug messages enabled");
	}

	if (opt.isSet(OPT_LONG_PREFIX BENCHMARK_KEYWORD))
	{
		tapkee::LoggingSingleton::instance().enable_benchmark();
		tapkee::LoggingSingleton::instance().message_info("Benchmarking enabled");
	}
		
	tapkee::ParametersMap parameters;
	{
		string method;
		opt.get(OPT_LONG_PREFIX METHOD_KEYWORD)->getString(method);
		tapkee::TAPKEE_METHOD tapkee_method = parse_reduction_method(method.c_str());
		if (tapkee_method==tapkee::UNKNOWN_METHOD)
		{
			tapkee::LoggingSingleton::instance().message_error(string("Unknown method ") + method);
			return 0;
		}
		else
			parameters[tapkee::REDUCTION_METHOD] = tapkee_method;
	}
	{
		string method;
		opt.get(OPT_LONG_PREFIX NEIGHBORS_METHOD_KEYWORD)->getString(method);
		tapkee::TAPKEE_NEIGHBORS_METHOD tapkee_neighbors_method = parse_neighbors_method(method.c_str());
		if (tapkee_neighbors_method==tapkee::UNKNOWN_NEIGHBORS_METHOD)
		{
			tapkee::LoggingSingleton::instance().message_error(string("Unknown neighbors method ") + method);
			return 0;
		}
		else
			parameters[tapkee::NEIGHBORS_METHOD] = tapkee_neighbors_method;
	}
	{
		string method;
		opt.get(OPT_LONG_PREFIX EIGEN_METHOD_KEYWORD)->getString(method);
		tapkee::TAPKEE_EIGEN_EMBEDDING_METHOD tapkee_eigen_method = parse_eigen_method(method.c_str());
		if (tapkee_eigen_method==tapkee::UNKNOWN_EIGEN_METHOD)
		{
			tapkee::LoggingSingleton::instance().message_error(string("Unknown eigendecomposition method ") + method);
			return 0;
		}
		else
			parameters[tapkee::EIGEN_EMBEDDING_METHOD] = tapkee_eigen_method;
	}
	{
		int target_dimension = 1;
		opt.get(OPT_LONG_PREFIX TARGET_DIMENSION_KEYWORD)->getInt(target_dimension);
		if (target_dimension < 0)
		{
			tapkee::LoggingSingleton::instance().message_error("Negative target dimensionality is not possible in current circumstances. "
			                                                   "Please visit other universe");
			return 0;
		}
		else
			parameters[tapkee::TARGET_DIMENSION] = static_cast<tapkee::IndexType>(target_dimension);
	}
	{
		int k = 1;
		opt.get(OPT_LONG_PREFIX NUM_NEIGHBORS_KEYWORD)->getInt(k);
		if (k < 3)
		{
			tapkee::LoggingSingleton::instance().message_error("The provided number of neighbors is too small, consider at least 10.");
			return 0;
		}
		else
			parameters[tapkee::NUMBER_OF_NEIGHBORS] = static_cast<tapkee::IndexType>(k);
	}
	{
		double width = 1.0;
		opt.get(OPT_LONG_PREFIX GAUSSIAN_WIDTH_KEYWORD)->getDouble(width);
		if (width < 0.0) 
		{
			tapkee::LoggingSingleton::instance().message_error("Width of the gaussian kernel is negative.");
			return 0;
		}
		else 
			parameters[tapkee::GAUSSIAN_KERNEL_WIDTH] = static_cast<tapkee::ScalarType>(width);
	}
	{
		int timesteps = 1;
		opt.get(OPT_LONG_PREFIX TIMESTEPS_KEYWORD)->getInt(timesteps);
		if (timesteps < 0)
		{
			tapkee::LoggingSingleton::instance().message_error("Number of timesteps is negative.");
			return 0;
		}
		else
			parameters[tapkee::DIFFUSION_MAP_TIMESTEPS] = static_cast<tapkee::IndexType>(timesteps);
	}
	{
		double eigenshift = 1e-9;
		opt.get(OPT_LONG_PREFIX EIGENSHIFT_KEYWORD)->getDouble(eigenshift);
		parameters[tapkee::EIGENSHIFT] = static_cast<tapkee::ScalarType>(eigenshift);
	}
	{
		double landmark_ratio = 0.0;
		opt.get(OPT_LONG_PREFIX LANDMARK_RATIO_KEYWORD)->getDouble(landmark_ratio);
		parameters[tapkee::LANDMARK_RATIO] = static_cast<tapkee::ScalarType>(landmark_ratio);
	}
	{
		if (opt.isSet(OPT_LONG_PREFIX SPE_LOCAL_KEYWORD))
			parameters[tapkee::SPE_GLOBAL_STRATEGY] = static_cast<bool>(false);
		else
			parameters[tapkee::SPE_GLOBAL_STRATEGY] = static_cast<bool>(true);
	}
	{
		double spe_tolerance = 1e-5;
		opt.get(OPT_LONG_PREFIX SPE_TOLERANCE_KEYWORD)->getDouble(spe_tolerance);
		parameters[tapkee::SPE_TOLERANCE] = static_cast<tapkee::ScalarType>(spe_tolerance);
	}
	{
		int spe_num_updates = 100;
		opt.get(OPT_LONG_PREFIX SPE_NUM_UPDATES_KEYWORD)->getInt(spe_num_updates);
		parameters[tapkee::SPE_NUM_UPDATES] = static_cast<tapkee::IndexType>(spe_num_updates);
	}
	{
		int max_iters = 1000;
		opt.get(OPT_LONG_PREFIX MAX_ITERS_KEYWORD)->getInt(max_iters);
		parameters[tapkee::MAX_ITERATION] = static_cast<tapkee::IndexType>(max_iters);
	}
	{
		double fa_epsilon = 1e-5;
		opt.get(OPT_LONG_PREFIX FA_EPSILON_KEYWORD)->getDouble(fa_epsilon);
		parameters[tapkee::FA_EPSILON] = static_cast<tapkee::ScalarType>(fa_epsilon);
	}
	
	{
		double perplexity = 30.0;
		opt.get(OPT_LONG_PREFIX SNE_PERPLEXITY_KEYWORD)->getDouble(perplexity);
		parameters[tapkee::SNE_PERPLEXITY] = static_cast<tapkee::ScalarType>(perplexity);
	}
	{
		double theta = 0.5;
		opt.get(OPT_LONG_PREFIX SNE_THETA_KEYWORD)->getDouble(theta);
		parameters[tapkee::SNE_THETA] = static_cast<tapkee::ScalarType>(theta);
	}

	parameters[tapkee::OUTPUT_FEATURE_VECTORS_ARE_COLUMNS] = true;
	parameters[tapkee::CANCEL_FUNCTION] = static_cast<bool (*)()>(cancel);
	// Load data
	string input_filename;
	string output_filename;
	if (!opt.isSet(OPT_LONG_PREFIX INPUT_FILE_KEYWORD))
	{
		tapkee::LoggingSingleton::instance().message_error("No input file specified. Please use " OPT_PREFIX "h flag if stucked");
		return 0;
	}
	else
		opt.get(OPT_LONG_PREFIX INPUT_FILE_KEYWORD)->getString(input_filename);

	if (!opt.isSet(OPT_LONG_PREFIX OUTPUT_FILE_KEYWORD))
	{
		tapkee::LoggingSingleton::instance().message_warning("No output file specified, using /dev/tty");
		output_filename = "/dev/tty";
	}
	else
		opt.get(OPT_LONG_PREFIX OUTPUT_FILE_KEYWORD)->getString(output_filename);

	bool output_projection = false;
	std::string output_matrix_filename = "/dev/null";
	std::string output_mean_filename = "/dev/null";
	if (opt.isSet(OPT_LONG_PREFIX OUTPUT_PROJECTION_MATRIX_FILE_KEYWORD) &&
		opt.isSet(OPT_LONG_PREFIX OUTPUT_PROJECTION_MEAN_FILE_KEYWORD))
	{
		output_projection = true;
		opt.get(OPT_LONG_PREFIX OUTPUT_PROJECTION_MATRIX_FILE_KEYWORD)->getString(output_matrix_filename);
		opt.get(OPT_LONG_PREFIX OUTPUT_PROJECTION_MEAN_FILE_KEYWORD)->getString(output_mean_filename);
	}

	ifstream ifs(input_filename.c_str());
	ofstream ofs(output_filename.c_str());
	ofstream ofs_matrix(output_matrix_filename.c_str());
	ofstream ofs_mean(output_matrix_filename.c_str());

	tapkee::DenseMatrix input_data = read_data(ifs);
	if (opt.isSet(OPT_LONG_PREFIX TRANSPOSE_INPUT_KEYWORD))
		input_data.transposeInPlace();
	parameters[tapkee::CURRENT_DIMENSION] = static_cast<tapkee::IndexType>(input_data.rows());
	
	std::stringstream ss;
	ss << "Data contains " << input_data.cols() << " feature vectors with dimension of " << input_data.rows();
	tapkee::LoggingSingleton::instance().message_info(ss.str());
	
	tapkee::ReturnResult embedding;

	vector<tapkee::IndexType> indices(input_data.cols());
	for (tapkee::IndexType i=0; i<input_data.cols(); ++i)
		indices[i] = i;

#ifdef USE_PRECOMPUTED
	tapkee::DenseMatrix distance_matrix;
	tapkee::DenseMatrix kernel_matrix;
	{
		tapkee::TAPKEE_METHOD method = parameters[tapkee::REDUCTION_METHOD].cast<tapkee::TAPKEE_METHOD>();
		if (method_needs_distance(method))
		{
			tapkee::tapkee_internal::timed_context context("[+] Distance matrix computation");
			distance_matrix = 
				matrix_from_callback(static_cast<tapkee::IndexType>(input_data.cols()),
				                     distance_callback(input_data));
		} 
		if (method_needs_kernel(method))
		{
			tapkee::tapkee_internal::timed_context context("[+] Kernel matrix computation");
			kernel_matrix = 
				matrix_from_callback(static_cast<tapkee::IndexType>(input_data.cols()),
				                     kernel_callback(input_data));
		}
	}
	precomputed_distance_callback dcb(distance_matrix);
	precomputed_kernel_callback kcb(kernel_matrix);
	feature_vector_callback fvcb(input_data);

	embedding = tapkee::embed(indices.begin(),indices.end(),kcb,dcb,fvcb,parameters);
#else
	distance_callback dcb(input_data);
	kernel_callback kcb(input_data);
	feature_vector_callback fvcb(input_data);

	embedding = tapkee::embed(indices.begin(),indices.end(),kcb,dcb,fvcb,parameters);
#endif
	// Save obtained data
	ofs << embedding.first;
	ofs.close();

	if (output_projection && embedding.second.implementation) {
		ofs_matrix << ((tapkee::MatrixProjectionImplementation*)embedding.second.implementation)->proj_mat;
		ofs_mean << ((tapkee::MatrixProjectionImplementation*)embedding.second.implementation)->mean_vec;
	}
	embedding.second.clear();
	ofs_matrix.close();
	ofs_mean.close();
	return 0;
#undef OPT_PREFIX
#undef OPT_LONG_PREFIX
}

int main(int argc, const char** argv)
{
	try 
	{
		return run(argc,argv);
	}
	catch (const std::exception& exc) 
	{
		std::cout << "Some error occured: " << exc.what() << std::endl;
	}
}
