/* Command line options for x265 */

HELP("Standalone Executable Options:")
OPT("help",            help,                            no_argument, 'h', "Show this help text")
OPT("cpuid",           cpuid,                     required_argument,   0, "Limit SIMD instructions 2:SSE2 .. 8:AVX2 (default:0-auto)")
OPT("threads",         param->poolNumThreads,     required_argument,   0, "Number of threads for thread pool (default:CPU core count)")
OPT("log",             param->logLevel,           required_argument,   0, "Logging level 0:ERROR 1:WARNING 2:INFO(default) 3:DEBUG -1:NONE")
STROPT("output",       bitstreamfn,               required_argument, 'o', "Bitstream output file name")
#if HIGH_BIT_DEPTH
OPT("depth",           param->internalBitDepth,   required_argument,   0, "Bit-depth of pixels within encoder (default:input-depth)")
#endif

HELP("Input Options:")
STROPT("input",        inputfn,                   required_argument,   0, "Raw YUV or Y4M input file name")
OPT("input-depth",     this->inputBitDepth,       required_argument,   0, "Bit-depth of input file (YUV only) (default: 8)")
OPT("width",           param->iSourceWidth,       required_argument,   0, "Source picture width, auto-detect if Y4M")
OPT("height",          param->iSourceHeight,      required_argument,   0, "Source picture height, auto-detect if Y4M")
OPT("rate",            param->iFrameRate,         required_argument,   0, "Frame rate, auto-detect if Y4M")
OPT("frame-skip",      this->frameSkip,           required_argument,   0, "Number of frames to skip at start of input YUV")
OPT("frames",          this->framesToBeEncoded,   required_argument, 'f', "Number of frames to be encoded (default=all)")

HELP("Reconstructed video options (debugging):")
STROPT("recon",        reconfn,                   required_argument, 'r', "Reconstructed image YUV or Y4M output file name")
OPT("recon-depth",     this->outputBitDepth,      required_argument,   0, "Bit-depth of output file (default:InternalBitDepth)")

HELP("Quad-Tree analysis:")
OPT("wpp",             param->iWaveFrontSynchro,               no_argument, 0, "Enable Wavefront Parallel Processing")
OPT("no-wpp",          param->iWaveFrontSynchro,               no_argument, 0, "Disable Wavefront Parallel Processing")
OPT("ctu",             param->uiMaxCUSize,               required_argument, 's', "Maximum CU size (default: 64x64)")
OPT("cu-depth",        param->uiMaxCUDepth,              required_argument, 'd', "Maximum CU recursion depth (default: 4)")
OPT("tu-maxlog2",      param->uiQuadtreeTULog2MaxSize,   required_argument, 0, "Maximum TU size in logarithm base 2")
OPT("tu-minlog2",      param->uiQuadtreeTULog2MinSize,   required_argument, 0, "Minimum TU size in logarithm base 2")
OPT("tu-intra-depth",  param->uiQuadtreeTUMaxDepthIntra, required_argument, 0, "Max TU recursive depth for intra CUs")
OPT("tu-inter-depth",  param->uiQuadtreeTUMaxDepthInter, required_argument, 0, "Max TU recursive depth for inter CUs")

HELP("Temporal / motion search options:")
OPT("me",              param->searchMethod,              required_argument, 0, "Motion search method 0:dia 1:hex 2:umh 3:star(default) 4:hm-orig")
OPT("merange",         param->iSearchRange,              required_argument, 0, "Motion search range (default: 64)")
OPT("bpredrange",      param->bipredSearchRange,         required_argument, 0, "Motion search range for bipred refinement (default:4)")
OPT("rect",            param->enableRectInter,                 no_argument, 0, "Enable rectangular motion partitions Nx2N and 2NxN")
OPT("no-rect",         param->enableRectInter,                 no_argument, 0, "Disable rectangular motion partitions Nx2N and 2NxN")
OPT("amp",             param->enableAMP,                       no_argument, 0, "Enable asymmetric motion partitions, requires --rect")
OPT("no-amp",          param->enableAMP,                       no_argument, 0, "Disable asymmetric motion partitions")
OPT("max-merge",       param->maxNumMergeCand,           required_argument, 0, "Maximum number of merge candidates (default: 5)")
OPT("fdm",             param->useFastDecisionForMerge,         no_argument, 0, "Enable fast decision for Merge RD Cost")
OPT("no-fdm",          param->useFastDecisionForMerge,         no_argument, 0, "Disable fast decision for Merge RD Cost")
OPT("early-skip",      param->useEarlySkipDetection,           no_argument, 0, "Enable early SKIP detection")
OPT("no-early-skip",   param->useEarlySkipDetection,           no_argument, 0, "Disable early SKIP detection")
OPT("merge-level",     param->log2ParallelMergeLevel,    required_argument, 0, "Parallel merge estimation region")
OPT("tmvp",            param->TMVPModeId,                required_argument, 0, "TMVP mode 0:disabled 1:enabled(default) 2:auto")
OPT("fast-cbf",        param->bUseCbfFastMode,                 no_argument, 0, "Enable Cbf fast mode")
OPT("no-fast-cbf",     param->bUseCbfFastMode,                 no_argument, 0, "Disable Cbf fast mode")

HELP("Spatial / intra options:")
OPT("rdpenalty",       param->rdPenalty,                 required_argument, 0, "penalty for 32x32 intra TU in non-I slices. 0:disabled 1:RD-penalty 2:maximum")
OPT("tskip",           param->useTransformSkip,                no_argument, 0, "Enable intra transform skipping")
OPT("no-tskip",        param->useTransformSkip,                no_argument, 0, "Disable intra transform skipping")
OPT("tskip-fast",      param->useTransformSkipFast,            no_argument, 0, "Enable fast intra transform skipping")
OPT("no-tskip-fast",   param->useTransformSkipFast,            no_argument, 0, "Disable fast intra transform skipping")
OPT("strong-intra-smoothing", param->useStrongIntraSmoothing,  no_argument, 0, "Enable strong intra smoothing for 32x32 blocks")
OPT("no-strong-intra-smoothing", param->useStrongIntraSmoothing, no_argument, 0, "Disable strong intra smoothing for 32x32 blocks")
OPT("constrained-intra", param->bUseConstrainedIntraPred,      no_argument, 0, "Constrained intra prediction (use only intra coded reference pixels)")
OPT("no-constrained-intra", param->bUseConstrainedIntraPred,   no_argument, 0, "Disable constrained intra prediction (use only intra coded reference pixels)")

HELP("Slice decision options:")
OPT("keyint",          param->iIntraPeriod,              required_argument, 'i', "Intra period in frames, (-1: only first frame)")
OPT("weightp",         param->useWeightedPred,                 no_argument, 'w', "Enable weighted prediction in P slices")
OPT("no-weightp",      param->useWeightedPred,                 no_argument,   0, "Disable weighted prediction in P slices")
OPT("weightbp",        param->useWeightedBiPred,               no_argument,   0, "Enable weighted (bidirectional) prediction in B slices")
OPT("no-weightbp",     param->useWeightedBiPred,               no_argument,   0, "Disable weighted (bidirectional) prediction in B slices")

HELP("QP and rate distortion options:")
OPT("qp",              param->iQP,                       required_argument, 'q', "Base QP for CQP mode (default: 30)")
OPT("cbqpoffs",        param->cbQpOffset,                required_argument, 0, "Chroma Cb QP Offset")
OPT("crqpoffs",        param->crQpOffset,                required_argument, 0, "Chroma Cr QP Offset")
OPT("aqselect",        param->bUseAdaptQpSelect,               no_argument, 0, "Adaptive QP selection")
OPT("aq",              param->bUseAdaptiveQP,                  no_argument, 0, "Enable QP adaptation based on a psycho-visual model")
OPT("no-aq",           param->bUseAdaptiveQP,                  no_argument, 0, "Disable QP adaptation based on a psycho-visual model")
OPT("aqrange",         param->iQPAdaptationRange,        required_argument, 0, "QP adaptation range")
OPT("cu-dqp-depth",    param->iMaxCuDQPDepth,            required_argument, 0, "Max depth for a minimum CU dQP")
OPT("rdoq",            param->useRDOQ,                         no_argument, 0, "Enable RDO quantization")
OPT("no-rdoq",         param->useRDOQ,                         no_argument, 0, "Disable RDO quantization")
OPT("rdoqts",          param->useRDOQTS,                       no_argument, 0, "Enable RDO quantization with transform skip")
OPT("no-rdoqts",       param->useRDOQTS,                       no_argument, 0, "Disable RDO quantization with transform skip")
OPT("signhide",        param->signHideFlag,                    no_argument, 0, "Hide sign bit of one coeff per TU (rdo)")
OPT("no-signhide",     param->signHideFlag,                    no_argument, 0, "Disable hide sign bit of one coeff per TU (rdo)")

HELP("Sample Adaptive Offset loop filter:")
OPT("sao",             param->bUseSAO,                         no_argument, 0, "Enable Sample Adaptive Offset")
OPT("no-sao",          param->bUseSAO,                         no_argument, 0, "Disable Sample Adaptive Offset")
OPT("sao-max-offsets", param->maxNumOffsetsPerPic,       required_argument, 0, "Max number of SAO offset per picture (Default: 2048)")
OPT("sao-lcu-bounds",  param->saoLcuBoundary,            required_argument, 0, "0: right/bottom boundary areas skipped  1: non-deblocked pixels are used")
OPT("sao-lcu-opt",     param->saoLcuBasedOptimization,   required_argument, 0, "0: SAO picture-based optimization, 1: SAO LCU-based optimization ")

HELP("SEI options:")
OPT("hash",            param->useDecodedPictureHashSEI,  required_argument, 0, "Decoded Picture Hash SEI 0: disabled, 1: MD5, 2: CRC, 3: Checksum ")
