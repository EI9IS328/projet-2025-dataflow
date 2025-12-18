//************************************************************************
//   proxy application v.0.0.1
//
//  semproxy.cpp: the main interface of  proxy application
//
//************************************************************************

#include "sem_proxy.h"

#include "filesystem"

#include <cartesian_struct_builder.h>
#include <cartesian_unstruct_builder.h>
#include <sem_solver_acoustic.h>
#include <source_and_receiver_utils.h>

#include <cxxopts.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <variant>
#include <ctime>
#include <limits>


using namespace SourceAndReceiverUtils;

void parseSismoPoints(std::string path, std::vector<std::array<float, 3>> * resultVector) {
  std::cout << "Parsing sismo receiver points in provided file : " << path << std::endl;
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Impossible to open the --sismo-points provided path ! " << std::endl;
    return;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream lineStringStream(line);
    std::array<float, 3> currentCoordinates;
    std::string token;
    int index = 0;
    while (std::getline(lineStringStream, token, ' ')) {
      currentCoordinates[index] = std::stof(token);
      index += 1;
    }
    if (index == 3) { // ensuring we parsed 3 values in this iteration of the loop
      resultVector->push_back(currentCoordinates);
    }
  }
  
}

SEMproxy::SEMproxy(const SemProxyOptions& opt)
{
  order = opt.order;
  snap_time_interval_ = opt.snap_time_interval;
  nb_elements_[0] = opt.ex;
  nb_elements_[1] = opt.ey;
  nb_elements_[2] = opt.ez;
  nb_nodes_[0] = opt.ex * order + 1;
  nb_nodes_[1] = opt.ey * order + 1;
  nb_nodes_[2] = opt.ez * order + 1;

  const float spongex = opt.boundaries_size;
  const float spongey = opt.boundaries_size;
  const float spongez = opt.boundaries_size;
  const std::array<float, 3> sponge_size = {spongex, spongey, spongez};
  src_coord_[0] = opt.srcx;
  src_coord_[1] = opt.srcy;
  src_coord_[2] = opt.srcz;

  domain_size_[0] = opt.lx;
  domain_size_[1] = opt.ly;
  domain_size_[2] = opt.lz;

  rcv_coord_[0] = opt.rcvx;
  rcv_coord_[1] = opt.rcvy;
  rcv_coord_[2] = opt.rcvz;

  bool isModelOnNodes = opt.isModelOnNodes;
  isElastic_ = opt.isElastic;
  cout << boolalpha;
  bool isElastic = isElastic_;


  is_snapshots_ =  opt.isSnapshotOn;

  is_compute_histogram_ = opt.isComputeHistogramOn;
  compute_histogram_interval = opt.computeHistogramInterval;

  const SolverFactory::methodType methodType = getMethod(opt.method);
  const SolverFactory::implemType implemType = getImplem(opt.implem);
  const SolverFactory::meshType meshType = getMesh(opt.mesh);
  const SolverFactory::modelLocationType modelLocation =
      isModelOnNodes ? SolverFactory::modelLocationType::OnNodes
                     : SolverFactory::modelLocationType::OnElements;
  const SolverFactory::physicType physicType = SolverFactory::physicType::Acoustic;

  float lx = domain_size_[0];
  float ly = domain_size_[1];
  float lz = domain_size_[2];
  int ex = nb_elements_[0];
  int ey = nb_elements_[1];
  int ez = nb_elements_[2];

  if (meshType == SolverFactory::Struct)
  {
    switch (order)
    {
      case 1: {
        model::CartesianStructBuilder<float, int, 1> builder(
            ex, lx, ey, ly, ez, lz, isModelOnNodes);
        m_mesh = builder.getModel();
        break;
      }
      case 2: {
        model::CartesianStructBuilder<float, int, 2> builder(
            ex, lx, ey, ly, ez, lz, isModelOnNodes);
        m_mesh = builder.getModel();
        break;
      }
      case 3: {
        model::CartesianStructBuilder<float, int, 3> builder(
            ex, lx, ey, ly, ez, lz, isModelOnNodes);
        m_mesh = builder.getModel();
        break;
      }
      default:
        throw std::runtime_error(
            "Order other than 1 2 3 is not supported (semproxy)");
    }
  }
  else if (meshType == SolverFactory::Unstruct)
  {
    model::CartesianParams<float, int> param(order, ex, ey, ez, lx, ly, lz,
                                             isModelOnNodes);
    model::CartesianUnstructBuilder<float, int> builder(param);
    m_mesh = builder.getModel();
  }
  else
  {
    throw std::runtime_error("Incorrect mesh type (SEMproxy ctor.)");
  }

  // time parameters
  if (opt.autodt)
  {
    float cfl_factor = (order == 2) ? 0.5 : 0.7;
    dt_ = find_cfl_dt(cfl_factor);
  }
  else
  {
    dt_ = opt.dt;
  }
  timemax_ = opt.timemax;
  num_sample_ = timemax_ / dt_;

  m_solver = SolverFactory::createSolver(methodType, implemType, meshType,
                                         modelLocation, physicType, order);
  m_solver->computeFEInit(*m_mesh, sponge_size, opt.surface_sponge,
                          opt.taper_delta);


  // Sismo points
  if (opt.sismoPoints.size() > 0) {
    // storing points in this->sismoPoints;
    parseSismoPoints(opt.sismoPoints, &sismoPoints);   
  }

  // find closest node to each of our receivers
  std::cout << "Looking for closest node to each of the provided sismo points receivers" << std::endl;
  for (int rcvIndex = 0; rcvIndex<sismoPoints.size(); rcvIndex++) {
    float minDist = INFINITY;
    float indexNodeMinDist = 0;
    for (int n = 0; n<m_mesh->getNumberOfNodes(); n++) {
      float tmpSum = 0.;
      for (int dim = 0; dim<3; dim++) {
        float nodeC = m_mesh->nodeCoord(n,dim);
        float receiverC = sismoPoints[rcvIndex][dim];
        tmpSum += (receiverC - nodeC) * (receiverC - nodeC);
      }
      float dist = sqrt(tmpSum);
      // update min variables
      if (dist < minDist) {minDist = dist; indexNodeMinDist = n;}
    }
    sismoPointsToNode.push_back(indexNodeMinDist); // save this node as the closest to the rcvIndex'th receiver
  }
  pnAtSismoPoints = allocateArray2D<arrayReal>(sismoPoints.size(), num_sample_, "pnAtSismoPoints");


  initFiniteElem();


  std::cout << "Number of node is " << m_mesh->getNumberOfNodes() << std::endl;
  std::cout << "Number of element is " << m_mesh->getNumberOfElements()
            << std::endl;
  std::cout << "Launching the Method " << opt.method << ", the implementation "
            << opt.implem << " and the mesh is " << opt.mesh << std::endl;
  std::cout << "Model is on " << (isModelOnNodes ? "nodes" : "elements")
            << std::endl;
  std::cout << "Physics type is " << (isElastic ? "elastic" : "acoustic")
            << std::endl;
  std::cout << "Order of approximation will be " << order << std::endl;
  std::cout << "Time step is " << dt_ << "s" << std::endl;
  std::cout << "Simulated time is " << timemax_ << "s" << std::endl;

}

void saveMetricsToFile(float kerneltime_ms,float outputtime_ms,float writesismotime_ms, float histotime_ms, float snapshottime_ms,
                        float * domain_size_, int * nb_elements_, int order) {
  // determine file name, create file
  std::ostringstream filename;
  std::time_t timestamp = std::time(nullptr);
  filename << timestamp << "-execution.csv";

  std::string fileNameStr = filename.str();
  std::ofstream file(fileNameStr);  
  // cols
  file << "timestamp,kerneltime,outputtime,writesismotime,histotime,snapshottime,ex,ey,ez,lx,ly,lz,order";
  file << std::endl;

  float lx = domain_size_[0];
  float ly = domain_size_[1];
  float lz = domain_size_[2];
  int ex = nb_elements_[0];
  int ey = nb_elements_[1];
  int ez = nb_elements_[2];

  file << timestamp;
  file <<","<<kerneltime_ms;
  file<<","<<outputtime_ms;
  file<<","<<writesismotime_ms;
  file<<","<<histotime_ms;
  file<<","<<snapshottime_ms;
  file<<","<<ex<<","<<ey<<","<<ez;
  file<<","<<lx<<","<<ly<<","<<lz;
  file<<","<<order;
  file << std::endl;
  file.close();

  std::cout << "Exec stats: " << fileNameStr << std::endl;
}

void SEMproxy::run()
{
  time_point<system_clock> startComputeTime, startOutputTime, totalComputeTime,
      totalOutputTime, startWriteSismoTime, totalWriteSismoTime, 
      startHistoTime, totalHistoTime, startSnapshotTime, totalSnapshotTime;

  SEMsolverDataAcoustic solverData(i1, i2, myRHSTerm, pnGlobal, rhsElement,
                                   rhsWeights);

  for (int indexTimeSample = 0; indexTimeSample < num_sample_;
       indexTimeSample++)
  {
    startComputeTime = system_clock::now();
    m_solver->computeOneStep(dt_, indexTimeSample, solverData);
    totalComputeTime += system_clock::now() - startComputeTime;

    startOutputTime = system_clock::now();

    if (indexTimeSample % 50 == 0)
    {
      m_solver->outputSolutionValues(indexTimeSample, i1, rhsElement[0],
                                     pnGlobal, "pnGlobal");
    }

    // Save pressure at receiver
    const int order = m_mesh->getOrder();

    float varnp1 = 0.0;
    for (int i = 0; i < order + 1; i++)
    {
      for (int j = 0; j < order + 1; j++)
      {
        for (int k = 0; k < order + 1; k++)
        {
          int nodeIdx = m_mesh->globalNodeIndex(rhsElementRcv[0], i, j, k);
          int globalNodeOnElement =
              i + j * (order + 1) + k * (order + 1) * (order + 1);
          varnp1 +=
              pnGlobal(nodeIdx, i2) * rhsWeightsRcv(0, globalNodeOnElement);
        }
      }
    }

    pnAtReceiver(0, indexTimeSample) = varnp1;

    for (int rcvIndex = 0; rcvIndex < sismoPoints.size(); rcvIndex++) {
        pnAtSismoPoints(rcvIndex, indexTimeSample) = pnGlobal(sismoPointsToNode[rcvIndex], i2);    
    }

    swap(i1, i2);

    auto tmp = solverData.m_i1;
    solverData.m_i1 = solverData.m_i2;
    solverData.m_i2 = tmp;

    totalOutputTime += system_clock::now() - startOutputTime;

    if (indexTimeSample % snap_time_interval_ == 0 && is_snapshots_ == true){
      startSnapshotTime = system_clock::now();
      saveSnapshot(indexTimeSample);
      totalSnapshotTime += system_clock::now() - startSnapshotTime;
    }
    if (indexTimeSample % compute_histogram_interval == 0 && is_compute_histogram_ == true) {
      startHistoTime = system_clock::now();
      computeHistogram(indexTimeSample);
      totalHistoTime += system_clock::now() - startHistoTime;
    }

  }

  // Save sismos for all receivers
  startWriteSismoTime = system_clock::now();
  for (int rcvIndex = 0; rcvIndex < sismoPoints.size(); rcvIndex++) {
    // create file and write in it
    std::ostringstream filename;
    filename << "../data/sismos/" << sismoPoints[rcvIndex][0] << "-" <<  sismoPoints[rcvIndex][1] << "-" << sismoPoints[rcvIndex][2] << "-sismo.txt";
    std::string fileNameStr = filename.str();
    std::ofstream file(fileNameStr);  
    
    for (int sample = 0; sample<num_sample_; sample++) {
      if (sample > 0) {file << " ";}
      file << pnAtSismoPoints(rcvIndex, sample);
    }
    file << std::endl;
    file.close();
    std::cout << "Wrote sismos in " << fileNameStr << std::endl;
  }
  totalWriteSismoTime += system_clock::now() - startWriteSismoTime;

  float kerneltime_ms = time_point_cast<microseconds>(totalComputeTime)
                            .time_since_epoch()
                            .count();
  float outputtime_ms =
      time_point_cast<microseconds>(totalOutputTime).time_since_epoch().count();

  float writesismotime_ms = time_point_cast<microseconds>(totalWriteSismoTime).time_since_epoch().count();

  float histotime_ms = time_point_cast<microseconds>(totalHistoTime).time_since_epoch().count();
  float snapshottime_ms = time_point_cast<microseconds>(totalSnapshotTime).time_since_epoch().count();

  cout << "------------------------------------------------ " << endl;
  cout << "\n---- Elapsed Kernel Time : " << kerneltime_ms / 1E6 << " seconds."
       << endl;
  cout << "---- Elapsed Output Time : " << outputtime_ms / 1E6 << " seconds."
       << endl;
  cout << "---- Elapsed write sismo time : " << writesismotime_ms / 1E6 << " seconds." << endl;
  cout << "------------------------------------------------ " << endl;

  saveMetricsToFile(kerneltime_ms, outputtime_ms, writesismotime_ms, histotime_ms, snapshottime_ms, domain_size_,nb_elements_, order);

}

// Initialize arrays
void SEMproxy::init_arrays()
{
  cout << "Allocate host memory for source and pressure values ..." << endl;

  rhsElement = allocateVector<vectorInt>(myNumberOfRHS, "rhsElement");
  rhsWeights = allocateArray2D<arrayReal>(
      myNumberOfRHS, m_mesh->getNumberOfPointsPerElement(), "RHSWeight");
  myRHSTerm = allocateArray2D<arrayReal>(myNumberOfRHS, num_sample_, "RHSTerm");
  pnGlobal =
      allocateArray2D<arrayReal>(m_mesh->getNumberOfNodes(), 2, "pnGlobal");
  pnAtReceiver = allocateArray2D<arrayReal>(1, num_sample_, "pnAtReceiver");
  // Receiver
  rhsElementRcv = allocateVector<vectorInt>(1, "rhsElementRcv");
  rhsWeightsRcv = allocateArray2D<arrayReal>(
      1, m_mesh->getNumberOfPointsPerElement(), "RHSWeightRcv");
}

// Initialize sources
void SEMproxy::init_source()
{
  arrayReal myRHSLocation = allocateArray2D<arrayReal>(1, 3, "RHSLocation");
  // std::cout << "All source are currently are coded on element 50." <<
  // std::endl;
  std::cout << "All source are currently are coded on middle element."
            << std::endl;
  int ex = nb_elements_[0];
  int ey = nb_elements_[1];
  int ez = nb_elements_[2];

  int lx = domain_size_[0];
  int ly = domain_size_[1];
  int lz = domain_size_[2];



  // Get source element index

  int source_index = floor((src_coord_[0] * ex) / lx) +
                     floor((src_coord_[1] * ey) / ly) * ex +
                     floor((src_coord_[2] * ez) / lz) * ey * ex;

  for (int i = 0; i < 1; i++)
  {
    rhsElement[i] = source_index;
  }

  // Get coordinates of the corners of the sourc element
  float cornerCoords[8][3];
  int I = 0;
  int nodes_corner[2] = {0, m_mesh->getOrder()};
  for (int k : nodes_corner)
  {
    for (int j : nodes_corner)
    {
      for (int i : nodes_corner)
      {
        int nodeIdx = m_mesh->globalNodeIndex(rhsElement[0], i, j, k);
        cornerCoords[I][0] = m_mesh->nodeCoord(nodeIdx, 0);
        cornerCoords[I][2] = m_mesh->nodeCoord(nodeIdx, 2);
        cornerCoords[I][1] = m_mesh->nodeCoord(nodeIdx, 1);
        I++;
      }
    }
  }

  // initialize source term
  vector<float> sourceTerm =
      myUtils.computeSourceTerm(num_sample_, dt_, f0, sourceOrder);
  for (int j = 0; j < num_sample_; j++)
  {
    myRHSTerm(0, j) = sourceTerm[j];
    if (j % 100 == 0)
      cout << "Sample " << j << "\t: sourceTerm = " << sourceTerm[j] << endl;
  }

  // get element number of source term
  myElementSource = rhsElement[0];
  cout << "Element number for the source location: " << myElementSource << endl
       << endl;

  int order = m_mesh->getOrder();

  switch (order)
  {
    case 1:
      SourceAndReceiverUtils::ComputeRHSWeights<1>(cornerCoords, src_coord_,
                                                   rhsWeights);
      break;
    case 2:
      SourceAndReceiverUtils::ComputeRHSWeights<2>(cornerCoords, src_coord_,
                                                   rhsWeights);
      break;
    case 3:
      SourceAndReceiverUtils::ComputeRHSWeights<3>(cornerCoords, src_coord_,
                                                   rhsWeights);
      break;
    default:
      throw std::runtime_error("Unsupported order: " + std::to_string(order));
  }

  // Receiver computation
  int receiver_index = floor((rcv_coord_[0] * ex) / lx) +
                       floor((rcv_coord_[1] * ey) / ly) * ex +
                       floor((rcv_coord_[2] * ez) / lz) * ey * ex;

  for (int i = 0; i < 1; i++)
  {
    rhsElementRcv[i] = receiver_index;
  }

  // Get coordinates of the corners of the receiver element
  float cornerCoordsRcv[8][3];
  I = 0;
  for (int k : nodes_corner)
  {
    for (int j : nodes_corner)
    {
      for (int i : nodes_corner)
      {
        int nodeIdx = m_mesh->globalNodeIndex(rhsElementRcv[0], i, j, k);
        cornerCoordsRcv[I][0] = m_mesh->nodeCoord(nodeIdx, 0);
        cornerCoordsRcv[I][2] = m_mesh->nodeCoord(nodeIdx, 2);
        cornerCoordsRcv[I][1] = m_mesh->nodeCoord(nodeIdx, 1);
        I++;
      }
    }
  }

  switch (order)
  {
    case 1:
      SourceAndReceiverUtils::ComputeRHSWeights<1>(cornerCoordsRcv, rcv_coord_,
                                                   rhsWeightsRcv);
      break;
    case 2:
      SourceAndReceiverUtils::ComputeRHSWeights<2>(cornerCoordsRcv, rcv_coord_,
                                                   rhsWeightsRcv);
      break;
    case 3:
      SourceAndReceiverUtils::ComputeRHSWeights<3>(cornerCoordsRcv, rcv_coord_,
                                                   rhsWeightsRcv);
      break;
    default:
      throw std::runtime_error("Unsupported order: " + std::to_string(order));
  }
}

SolverFactory::implemType SEMproxy::getImplem(string implemArg)
{
  if (implemArg == "makutu") return SolverFactory::MAKUTU;
  if (implemArg == "shiva") return SolverFactory::SHIVA;

  throw std::invalid_argument(
      "Implentation type does not follow any valid type.");
}

SolverFactory::meshType SEMproxy::getMesh(string meshArg)
{
  if (meshArg == "cartesian") return SolverFactory::Struct;
  if (meshArg == "ucartesian") return SolverFactory::Unstruct;

  std::cout << "Mesh type found is " << meshArg << std::endl;
  throw std::invalid_argument("Mesh type does not follow any valid type.");
}

SolverFactory::methodType SEMproxy::getMethod(string methodArg)
{
  if (methodArg == "sem") return SolverFactory::SEM;
  if (methodArg == "dg") return SolverFactory::DG;

  throw std::invalid_argument("Method type does not follow any valid type.");
}

float SEMproxy::find_cfl_dt(float cfl_factor)
{
  float sqrtDim3 = 1.73;  // to change for 2d
  float min_spacing = m_mesh->getMinSpacing();
  float v_max = m_mesh->getMaxSpeed();

  float dt = cfl_factor * min_spacing / (sqrtDim3 * v_max);

  return dt;
}

char* float_to_cstring(float f) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << f;

    std::string tmp = oss.str();
    std::replace(tmp.begin(), tmp.end(), '.', ',');

    // Allouer un buffer char*
    char* result = new char[tmp.size() + 1];
    std::strcpy(result, tmp.c_str());
    return result;
}

std::filesystem::path executableDir() {
    return std::filesystem::path(
        std::filesystem::canonical("/proc/self/exe")
    ).parent_path();
}

void SEMproxy::computeHistogram(int timestep) {
  // min et max
  float min = std::numeric_limits<float>::infinity();
  float max = -std::numeric_limits<float>::infinity();
  for (int n = 0; n<m_mesh->getNumberOfNodes(); n++) {
      float value = pnGlobal(n, 1);
      min = fmin(value, min);
      max = fmax(value, max);
  }

  int nb_bins = 10;
  // calculer les bin edges
  std::vector<float> bin_edges;
  bin_edges.push_back(min);
  float bin_width = (max-min)/nb_bins;
  for (int i = 1; i<=nb_bins; i++) {
    bin_edges.push_back(min + i*bin_width);
  }

  // calculer l'histogramme
  std::vector<int> hist(nb_bins, 0);
  if (min == max) {
    hist[0] = m_mesh->getNumberOfNodes();    
  }
  else{
    for (int n = 0; n<m_mesh->getNumberOfNodes(); n++) {
        float value = pnGlobal(n, 1);
        int idx = ((value - min) / bin_width);
        if (idx == nb_bins) {
          hist[nb_bins-1]++;
        } 
        else {hist[idx]++;}
    }
  }

  // save 
  std::filesystem::path baseDir = executableDir();

  std::filesystem::path filename = baseDir / ("../../data/histo/histo_" + std::to_string(timestep)
                         + "_order" + std::to_string(order) + ".bin");
  std::ofstream out(filename);
  if (!out) {
      std::cerr << "Error opening file " << filename<< ": " << std::strerror(errno) << "\n";
      return;
  }
  out << "bin_edges:\n";
  for (int i = 0; i<nb_bins+1; i++) {
    out <<bin_edges[i] << ' ';
  }
  out << '\n';
  out << "hist:\n";
  for (int i = 0; i<nb_bins; i++) {
    out << hist[i]<< ' ';
  }
  out << '\n';
  out.close();
}

void SEMproxy::saveSnapshot(int timestep){
  std::filesystem::path baseDir = executableDir();

  std::filesystem::path filename = baseDir /
      ("../../data/snapshot/snapshot_" +
      std::to_string(timestep) +
      "_order" + std::to_string(order) +
      ".bin");

  std::ofstream out(filename);
  if (!out) {
      std::cerr << "Error opening file " << filename<< ": " << std::strerror(errno) << "\n";
      return;
  }

  // Parcours des noeuds
  for (int n = 0; n < m_mesh->getNumberOfNodes(); n++) {
    if ( m_mesh->nodeCoord(n,0) == 0 && n != 0 ){
      out << "\n";
    }
      float value = pnGlobal(n, 1);
      out << value;
      out << " ";
  }

  out.close();
  // std::cout << "Snapshot saved: " << filename << "\n";

}


/*
for (int n = 0; n<m_mesh->getNumberOfNodes(); n++) {
      float tmpSum = 0.;
      for (int dim = 0; dim<3; dim++) {
        int nodeC = m_mesh->nodeCoord(n,dim);
        int receiverC = sismoPoints[rcvIndex][dim];
        tmpSum += (receiverC - nodeC) * (receiverC - nodeC);
      }
      float dist = sqrt(tmpSum);
      // update min variables
      if (dist < minDist) {minDist = dist; indexNodeMinDist = n;}
    }*/