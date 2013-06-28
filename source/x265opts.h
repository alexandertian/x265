/* Command line options for x265 */

HELP("Standalone Executable Options:")
OPT("help",            help,                            no_argument, 'h', "Show this help text")
OPT("cpuid",           cpuid,                     required_argument, 0, "Limit SIMD instructions 2:SSE2 .. 8:AVX2 (default:0-auto)")
OPT("threads",         param->poolNumThreads,     required_argument, 0, "Number of threads for thread pool (default:CPU core count)")
OPT("gops",            param->gopNumThreads,      required_argument, 0, "Number of GOPS to encode in parallel (default:1)")
OPT("log",             param->logLevel,           required_argument, 0, "Logging level 0:ERROR 1:WARNING 2:INFO(default) 3:DEBUG -1:NONE")
STROPT("output",       bitstreamfn,               required_argument, 'o', "Bitstream output file name")
#if HIGH_BIT_DEPTH
OPT("depth",           param->internalBitDepth,   required_argument, 0, "Bit-depth of pixels within encoder (default:input-depth)")
#endif

HELP("Input Options:")
STROPT("input",        inputfn,                   required_argument, 0, "Raw YUV or Y4M input file name")
OPT("input-depth",     inputBitDepth,             required_argument, 0, "Bit-depth of input file (YUV only) (default: 8)")
OPT("width",           param->sourceWidth,        required_argument, 0, "Source picture width, auto-detect if Y4M")
OPT("height",          param->sourceHeight,       required_argument, 0, "Source picture height, auto-detect if Y4M")
OPT("rate",            param->frameRate,          required_argument, 0, "Frame rate, auto-detect if Y4M")
OPT("frame-skip",      this->frameSkip,           required_argument, 0, "Number of frames to skip at start of input file")
OPT("frames",          this->framesToBeEncoded,   required_argument, 'f', "Number of frames to be encoded (default=all)")

HELP("Reconstructed video options (debugging):")
STROPT("recon",        reconfn,                   required_argument, 'r', "Reconstructed image YUV or Y4M output file name")
OPT("recon-depth",     outputBitDepth,            required_argument, 0, "Bit-depth of output file (default:InternalBitDepth)")

HELP("Quad-Tree analysis:")
OPT("wpp",             param->bEnableWavefront,         no_argument, 0, "Enable Wavefront Parallel Processing")
OPT("no-wpp",          param->bEnableWavefront,         no_argument, 0, "Disable Wavefront Parallel Processing")
OPT("ctu",             param->maxCUSize,          required_argument, 's', "Maximum CU size (default: 64x64)")
OPT("cu-depth",        param->maxCUDepth,         required_argument, 'd', "Maximum CU recursion depth (default: 4)")
OPT("tu-maxlog2",      param->tuQTMaxLog2Size,    required_argument, 0, "Maximum TU size in logarithm base 2")
OPT("tu-minlog2",      param->tuQTMinLog2Size,    required_argument, 0, "Minimum TU size in logarithm base 2")
OPT("tu-intra-depth",  param->tuQTMaxIntraDepth,  required_argument, 0, "Max TU recursive depth for intra CUs")
OPT("tu-inter-depth",  param->tuQTMaxInterDepth,  required_argument, 0, "Max TU recursive depth for inter CUs")

HELP("Temporal / motion search options:")
OPT("me",              param->searchMethod,           required_argument, 0, "Motion search method 0:dia 1:hex 2:umh 3:star(default) 4:hm-orig 5:full")
OPT("merange",         param->searchRange,            required_argument, 0, "Motion search range (default: 64)")
OPT("bpredrange",      param->bipredSearchRange,      required_argument, 0, "Motion search range for bipred refinement (default:4)")
OPT("rect",            param->bEnableRectInter,             no_argument, 0, "Enable rectangular motion partitions Nx2N and 2NxN")
OPT("no-rect",         param->bEnableRectInter,             no_argument, 0, "Disable rectangular motion partitions Nx2N and 2NxN")
OPT("amp",             param->bEnableAMP,                   no_argument, 0, "Enable asymmetric motion partitions, requires --rect")
OPT("no-amp",          param->bEnableAMP,                   no_argument, 0, "Disable asymmetric motion partitions")
OPT("rdo",             param->bEnableRDO,                   no_argument, 0, "Enable rate distortion-based mode decision")
OPT("no-rdo",          param->bEnableRDO,                   no_argument, 0, "Enable mode decision without rate distortion optimization")
OPT("max-merge",       param->maxNumMergeCand,        required_argument, 0, "Maximum number of merge candidates (default: 5)")
OPT("fdm",             param->bEnableFastMergeDecision,     no_argument, 0, "Enable fast decision for Merge RD Cost")
OPT("no-fdm",          param->bEnableFastMergeDecision,     no_argument, 0, "Disable fast decision for Merge RD Cost")
OPT("early-skip",      param->bEnableEarlySkip,             no_argument, 0, "Enable early SKIP detection")
OPT("no-early-skip",   param->bEnableEarlySkip,             no_argument, 0, "Disable early SKIP detection")
OPT("merge-level",     param->log2ParallelMergeLevel, required_argument, 0, "Parallel merge estimation region")
OPT("tmvp",            param->TMVPModeId,             required_argument, 0, "TMVP mode 0:disabled 1:enabled(default) 2:auto")
OPT("fast-cbf",        param->bEnableCbfFastMode,           no_argument, 0, "Enable Cbf fast mode")
OPT("no-fast-cbf",     param->bEnableCbfFastMode,           no_argument, 0, "Disable Cbf fast mode")

HELP("Spatial / intra options:")
OPT("rdpenalty",       param->rdPenalty,              required_argument, 0, "penalty for 32x32 intra TU in non-I slices. 0:disabled 1:RD-penalty 2:maximum")
OPT("tskip",           param->bEnableTransformSkip,         no_argument, 0, "Enable intra transform skipping")
OPT("no-tskip",        param->bEnableTransformSkip,         no_argument, 0, "Disable intra transform skipping")
OPT("tskip-fast",      param->bEnableTSkipFast,             no_argument, 0, "Enable fast intra transform skipping")
OPT("no-tskip-fast",   param->bEnableTSkipFast,             no_argument, 0, "Disable fast intra transform skipping")
OPT("strong-intra-smoothing", param->bEnableStrongIntraSmoothing,  no_argument, 0, "Enable strong intra smoothing for 32x32 blocks")
OPT("no-strong-intra-smoothing", param->bEnableStrongIntraSmoothing, no_argument, 0, "Disable strong intra smoothing for 32x32 blocks")
OPT("constrained-intra", param->bEnableConstrainedIntra,    no_argument, 0, "Constrained intra prediction (use only intra coded reference pixels)")
OPT("no-constrained-intra", param->bEnableConstrainedIntra, no_argument, 0, "Disable constrained intra prediction (use only intra coded reference pixels)")

HELP("Slice decision options:")
OPT("keyint",          param->keyframeInterval,       required_argument, 'i', "Intra period in frames, (-1: only first frame)")
OPT("bframes",         param->bframes,                required_argument, 'b', "Maximum number of consecutive b-frames (now it only enables B GOP structure)")
OPT("weightp",         param->bEnableWeightedPred,          no_argument, 'w', "Enable weighted prediction in P slices")
OPT("no-weightp",      param->bEnableWeightedPred,          no_argument, 0, "Disable weighted prediction in P slices")
OPT("weightbp",        param->bEnableWeightedBiPred,        no_argument, 0, "Enable weighted (bidirectional) prediction in B slices")
OPT("no-weightbp",     param->bEnableWeightedBiPred,        no_argument, 0, "Disable weighted (bidirectional) prediction in B slices")

HELP("QP and rate distortion options:")
OPT("qp",              param->qp,                     required_argument, 'q', "Base QP for CQP mode (default: 30)")
OPT("cbqpoffs",        param->cbQpOffset,             required_argument, 0, "Chroma Cb QP Offset")
OPT("crqpoffs",        param->crQpOffset,             required_argument, 0, "Chroma Cr QP Offset")
OPT("aqselect",        param->bEnableAdaptQpSelect,         no_argument, 0, "Adaptive QP selection")
OPT("aq",              param->bEnableAdaptiveQP,            no_argument, 0, "Enable QP adaptation based on a psycho-visual model")
OPT("no-aq",           param->bEnableAdaptiveQP,            no_argument, 0, "Disable QP adaptation based on a psycho-visual model")
OPT("aqrange",         param->qpAdaptionRange,        required_argument, 0, "QP adaptation range")
OPT("cu-dqp-depth",    param->maxCUdQPDepth,          required_argument, 0, "Max depth for a minimum CU dQP")
OPT("rdoq",            param->bEnableRDOQ,                  no_argument, 0, "Enable RDO quantization")
OPT("no-rdoq",         param->bEnableRDOQ,                  no_argument, 0, "Disable RDO quantization")
OPT("rdoqts",          param->bEnableRDOQTS,                no_argument, 0, "Enable RDO quantization with transform skip")
OPT("no-rdoqts",       param->bEnableRDOQTS,                no_argument, 0, "Disable RDO quantization with transform skip")
OPT("signhide",        param->bEnableSignHiding,            no_argument, 0, "Hide sign bit of one coeff per TU (rdo)")
OPT("no-signhide",     param->bEnableSignHiding,            no_argument, 0, "Disable hide sign bit of one coeff per TU (rdo)")

HELP("Sample Adaptive Offset loop filter:")
OPT("sao",             param->bEnableSAO,                   no_argument, 0, "Enable Sample Adaptive Offset")
OPT("no-sao",          param->bEnableSAO,                   no_argument, 0, "Disable Sample Adaptive Offset")
OPT("sao-max-offsets", param->maxSAOOffsetsPerPic,    required_argument, 0, "Max number of SAO offset per picture (Default: 2048)")
OPT("sao-lcu-bounds",  param->saoLcuBoundary,         required_argument, 0, "0: right/bottom boundary areas skipped  1: non-deblocked pixels are used")
OPT("sao-lcu-opt",     param->saoLcuBasedOptimization,required_argument, 0, "0: SAO picture-based optimization, 1: SAO LCU-based optimization ")

HELP("SEI options:")
OPT("hash",            param->bEnableDecodedPictureHashSEI, required_argument, 0, "Decoded Picture Hash SEI 0: disabled, 1: MD5, 2: CRC, 3: Checksum ")
