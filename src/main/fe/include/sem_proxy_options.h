#pragma once

#include <cxxopts.hpp>
#include <stdexcept>
#include <string>

class SemProxyOptions
{
 public:
  // Defaults
  int order = 2;
  int ex = 50, ey = 50, ez = 50;
  float lx = 2000.f, ly = 2000.f, lz = 2000.f;
  float srcx = 1010.f, srcy = 1010.f, srcz = 1010.f;
  float rcvx = 1410.f, rcvy = 1010.f, rcvz = 1010.f;
  std::string implem = "makutu";  // makutu|shiva
  std::string method = "sem";     // sem|dg
  std::string mesh = "cartesian";
  float dt = 0.001;
  float timemax = 1.5;
  bool autodt = false;
  std::string sismoPoints = "";
  int snap_time_interval = 150;
  // sponge boundaries parameters
  float boundaries_size = 0;
  bool surface_sponge = false;
  float taper_delta = 0.015;
  // Boolean to tell if the model is charged on nodes or on element
  bool isModelOnNodes = false;
  bool isElastic = false;
  bool isSnapshotOn = false;
  bool isStatsAnalysisOn = false;
  int statsAnalysisInterval= 50;
  bool isComputeHistogramOn = false;
  bool isComputeFourierOn = false;
  int computeHistogramInterval = 150;
  int sliceSnapshotCoord = -1;
  bool saveSliceSnapshotToPPM = false; // if false save as bin

  void validate() const
  {
    if (order < 1) throw std::runtime_error("order must be >= 1");
    if (ex <= 0 || ey <= 0 || ez <= 0)
      throw std::runtime_error("ex/ey/ez must be > 0");
    if (lx <= 0 || ly <= 0 || lz <= 0)
      throw std::runtime_error("lx/ly/lz must be > 0");
  }

  // Bind CLI flags to this instance (no --help here)
  static void bind_cli(cxxopts::Options& opts, SemProxyOptions& o)
  {
    opts.add_options()("o,order", "Order of approximation",
                       cxxopts::value<int>(o.order))(
        "ex", "Number of elements on X (Cartesian mesh)",
        cxxopts::value<int>(o.ex))("ey",
                                   "Number of elements on Y (Cartesian mesh)",
                                   cxxopts::value<int>(o.ey))(
        "ez", "Number of elements on Z (Cartesian mesh)",
        cxxopts::value<int>(o.ez))("lx", "Domain size X (Cartesian)",
                                   cxxopts::value<float>(o.lx))(
        "ly", "Domain size Y (Cartesian)", cxxopts::value<float>(o.ly))(
        "lz", "Domain size Z (Cartesian)", cxxopts::value<float>(o.lz))(
        "implem", "Implementation: makutu|shiva",
        cxxopts::value<std::string>(o.implem))(
        "method", "Method: sem|dg", cxxopts::value<std::string>(o.method))(
        "mesh", "Mesh: cartesian|ucartesian",
        cxxopts::value<std::string>(o.mesh))(
        "dt", "Time step selection in s (default = 0.001s)",
        cxxopts::value<float>(o.dt))(
        "timemax", "Duration of the simulation in s (default = 1.5s)",
        cxxopts::value<float>(o.timemax))(
        "auto-dt", "Select automatique dt via CFL equation.",
        cxxopts::value<bool>(o.autodt))(
        "boundaries-size", "Size of absorbing boundaries (meters)",
        cxxopts::value<float>(o.boundaries_size))(
        "sponge-surface", "Considere the surface's nodes as non sponge nodes",
        cxxopts::value<bool>(o.surface_sponge))(
        "taper-delta", "Taper delta for sponge boundaries value",
        cxxopts::value<float>(o.taper_delta))(
        "is-model-on-nodes",
        "Boolean to tell if the model is charged on nodes (true) or on element "
        "(false)",
        cxxopts::value<bool>(o.isModelOnNodes))
        ("s,snapshot","Enable or disable saving snapshots",cxxopts::value<bool>(o.isSnapshotOn))
        ("sismo-points", "Path to sismo receptor points to save", cxxopts::value<std::string>(o.sismoPoints))
        ("sismo-fourier", "Path to sismo receptor to do fourier", cxxopts::value<bool>(o.isComputeFourierOn))
        ("is-elastic", "Elastic simulation", cxxopts::value<bool>(o.isElastic))
        ("stats-analysis", "Enable stats analysis(min ,max, variance, moyenne) during execution", cxxopts::value<bool>(o.isStatsAnalysisOn))
        ("stats-interval", "Delay between each stats analysis (step (ms) )", cxxopts::value<int>(o.statsAnalysisInterval))
        ("compute-histogram","Enable or disable computing histogram for pressure value distribution",cxxopts::value<bool>(o.isComputeHistogramOn))
        ("compute-histogram-delay", "Delay between each histogram computation (step (ms) )", cxxopts::value<int>(o.computeHistogramInterval))
        ("sd,snapshot-delay", "Delay between each snapshot (step (ms) )", cxxopts::value<int>(o.snap_time_interval))
        ("slice-snapshot", "Enable snapshots at given coordinates. Will use the --snapshot-delay parameter for delay", cxxopts::value<int>(o.sliceSnapshotCoord))
        ("slice-ppm", "Save slice snapshots as PPM format. Saving slice snapshots without this option result in binary save", cxxopts::value<bool>(o.saveSliceSnapshotToPPM))
        ;
  } 
};
