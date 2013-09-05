/* Command line options for x265 */

HELP("Standalone Executable Options:")
OPT("help",            help,                            no_argument, 'h', "Show this help text")
OPT("cpuid",           cpuid,                     required_argument, 0, "Limit SIMD arch 0:auto 1:None 2:SSE2 .. 8:AVX2")
OPT("threads",         param->poolNumThreads,     required_argument, 0, "Number of threads for thread pool (0: detect CPU core count)")
OPT("frame-threads",   param->frameNumThreads,    required_argument, 'F', "Number of concurrently encoded frames")
OPT("log",             param->logLevel,           required_argument, 0, "Logging level 0:ERROR 1:WARNING 2:INFO 3:DEBUG -1:NONE")
STROPT("csv",          csvfn,                     required_argument, 0, "`Comma separated value' log file, appends one line per run")
OPT("no-progress",     this->bProgress,                 no_argument, 0, "Disable progress reports")
STROPT("output",       bitstreamfn,               required_argument, 'o', "Bitstream output file name")
#if HIGH_BIT_DEPTH
OPT("depth",           param->internalBitDepth,   required_argument, 0, "Bit-depth of pixels within encoder")
#endif

HELP("Input Options:")
STROPT("input",        inputfn,                   required_argument, 0, "Raw YUV or Y4M input file name")
OPT("input-depth",     inputBitDepth,             required_argument, 0, "Bit-depth of input file (YUV only)")
STROPT("input-res",    inputRes,                  required_argument, 0, "Source picture size [w x h], auto-detect if Y4M")
OPT("fps",             param->frameRate,          required_argument, 0, "Frame rate, auto-detect if Y4M")
OPT("frame-skip",      this->frameSkip,           required_argument, 0, "Number of frames to skip at start of input file")
OPT("frames",          this->framesToBeEncoded,   required_argument, 'f', "Number of frames to be encoded (0 implies all)")

HELP("Reconstructed video options (debugging):")
STROPT("recon",        reconfn,                   required_argument, 'r', "Reconstructed image YUV or Y4M output file name")
OPT("recon-depth",     outputBitDepth,            required_argument, 0, "Bit-depth of output file")

HELP("Quad-Tree analysis:")
OPT("no-wpp",          param->bEnableWavefront,         no_argument, 0, "Disable Wavefront Parallel Processing")
OPT("wpp",             param->bEnableWavefront,         no_argument, 0, "Enable Wavefront Parallel Processing")
OPT("ctu",             param->maxCUSize,          required_argument, 's', "Maximum CU size (default: 64x64)")
OPT("tu-intra-depth",  param->tuQTMaxIntraDepth,  required_argument, 0, "Max TU recursive depth for intra CUs")
OPT("tu-inter-depth",  param->tuQTMaxInterDepth,  required_argument, 0, "Max TU recursive depth for inter CUs")

HELP("Temporal / motion search options:")
OPT("me",              param->searchMethod,           required_argument, 0, "Motion search method 0:dia 1:hex 2:umh 3:star 4:full")
OPT("merange",         param->searchRange,            required_argument, 0, "Motion search range")
OPT("bpredrange",      param->bipredSearchRange,      required_argument, 0, "Motion search range for bipred refinement")
OPT("no-rect",         param->bEnableRectInter,             no_argument, 0, "Disable rectangular motion partitions Nx2N and 2NxN")
OPT("rect",            param->bEnableRectInter,             no_argument, 0, "Enable rectangular motion partitions Nx2N and 2NxN")
OPT("no-amp",          param->bEnableAMP,                   no_argument, 0, "Disable asymmetric motion partitions")
OPT("amp",             param->bEnableAMP,                   no_argument, 0, "Enable asymmetric motion partitions, requires --rect")
OPT("no-rdo",          param->bEnableRDO,                   no_argument, 0, "Enable mode decision without rate distortion optimization")
OPT("rdo",             param->bEnableRDO,                   no_argument, 0, "Enable rate distortion-based mode decision")
OPT("max-merge",       param->maxNumMergeCand,        required_argument, 0, "Maximum number of merge candidates")
OPT("no-early-skip",   param->bEnableEarlySkip,             no_argument, 0, "Disable early SKIP detection")
OPT("early-skip",      param->bEnableEarlySkip,             no_argument, 0, "Enable early SKIP detection")
OPT("no-fast-cbf",     param->bEnableCbfFastMode,           no_argument, 0, "Disable Cbf fast mode")
OPT("fast-cbf",        param->bEnableCbfFastMode,           no_argument, 0, "Enable Cbf fast mode")

HELP("Spatial / intra options:")
OPT("rdpenalty",       param->rdPenalty,              required_argument, 0, "penalty for 32x32 intra TU in non-I slices. 0:disabled 1:RD-penalty 2:maximum")
OPT("no-tskip",        param->bEnableTransformSkip,         no_argument, 0, "Disable intra transform skipping")
OPT("tskip",           param->bEnableTransformSkip,         no_argument, 0, "Enable intra transform skipping")
OPT("no-tskip-fast",   param->bEnableTSkipFast,             no_argument, 0, "Disable fast intra transform skipping")
OPT("tskip-fast",      param->bEnableTSkipFast,             no_argument, 0, "Enable fast intra transform skipping")
OPT("no-strong-intra-smoothing", param->bEnableStrongIntraSmoothing, no_argument, 0, "Disable strong intra smoothing for 32x32 blocks")
OPT("strong-intra-smoothing", param->bEnableStrongIntraSmoothing,  no_argument, 0, "Enable strong intra smoothing for 32x32 blocks")
OPT("no-constrained-intra", param->bEnableConstrainedIntra, no_argument, 0, "Disable constrained intra prediction (use only intra coded reference pixels)")
OPT("constrained-intra", param->bEnableConstrainedIntra,    no_argument, 0, "Constrained intra prediction (use only intra coded reference pixels)")

HELP("Slice decision options:")
OPT("refresh",         param->decodingRefreshType,    required_argument, 0, "Intra refresh type - 0:none, 1:CDR, 2:IDR (default: CDR)")
OPT("keyint",          param->keyframeMax,            required_argument, 'i', "Max intra period in frames")
OPT("open-gop",        param->bOpenGOP,                     no_argument, 0, "Only use intra for very first picture")
OPT("rc-lookahead",    param->lookaheadDepth,         required_argument, 0, "Number of frames for frame-type lookahead (determines encoder latency)")
OPT("bframes",         param->bframes,                required_argument, 'b', "Maximum number of consecutive b-frames (now it only enables B GOP structure)")
OPT("bframe-bias",     param->bFrameBias,             required_argument, 0, "Bias towards B frame decisions")
// Disabled because weighted uni-prediction was busted by not using
// pre-generated planes in motion compensation
//OPT("no-weightp",      param->bEnableWeightedPred,          no_argument, 0, "Disable weighted prediction in P slices")
//OPT("weightp",         param->bEnableWeightedPred,          no_argument, 'w', "Enable weighted prediction in P slices")
// Disabled because weighted bi prediction is busted
//OPT("no-weightbp",     param->bEnableWeightedBiPred,        no_argument, 0, "Disable weighted (bidirectional) prediction in B slices")
//OPT("weightbp",        param->bEnableWeightedBiPred,        no_argument, 0, "Enable weighted (bidirectional) prediction in B slices")

HELP("QP, rate control and rate distortion options:")
OPT("bitrate",         param->rc.bitrate,             required_argument, 0, "Target bitrate (kbps), implies ABR")
OPT("qp",              param->rc.qp,                  required_argument, 'q', "Base QP for CQP mode")
OPT("cbqpoffs",        param->cbQpOffset,             required_argument, 0, "Chroma Cb QP Offset")
OPT("crqpoffs",        param->crQpOffset,             required_argument, 0, "Chroma Cr QP Offset")
OPT("no-rdoq",         param->bEnableRDOQ,                  no_argument, 0, "Disable RDO quantization")
OPT("rdoq",            param->bEnableRDOQ,                  no_argument, 0, "Enable RDO quantization")
OPT("no-rdoqts",       param->bEnableRDOQTS,                no_argument, 0, "Disable RDO quantization with transform skip")
OPT("rdoqts",          param->bEnableRDOQTS,                no_argument, 0, "Enable RDO quantization with transform skip")
OPT("no-signhide",     param->bEnableSignHiding,            no_argument, 0, "Disable hide sign bit of one coeff per TU (rdo)")
OPT("signhide",        param->bEnableSignHiding,            no_argument, 0, "Hide sign bit of one coeff per TU (rdo)")

HELP("Loop filter:")
OPT("no-lft",          param->bEnableLoopFilter,            no_argument, 0, "Disable Loop Filter")
OPT("lft",             param->bEnableLoopFilter,            no_argument, 0, "Enable Loop Filter")

HELP("Sample Adaptive Offset loop filter:")
OPT("no-sao",          param->bEnableSAO,                   no_argument, 0, "Disable Sample Adaptive Offset")
OPT("sao",             param->bEnableSAO,                   no_argument, 0, "Enable Sample Adaptive Offset")
OPT("sao-lcu-bounds",  param->saoLcuBoundary,         required_argument, 0, "0: right/bottom boundary areas skipped  1: non-deblocked pixels are used")
OPT("sao-lcu-opt",     param->saoLcuBasedOptimization,required_argument, 0, "0: SAO picture-based optimization, 1: SAO LCU-based optimization ")

HELP("SEI options:")
OPT("hash",            param->bEnableDecodedPictureHashSEI, required_argument, 0, "Decoded Picture Hash SEI 0: disabled, 1: MD5, 2: CRC, 3: Checksum ")
