*********
Threading
*********

Thread Pool
===========

x265 creates a pool of worker threads and shares this thread pool
with all encoders within the same process (it is process global, aka a
singleton).  The number of threads within the thread pool is determined
by the encoder which first allocates the pool, which by definition is
the first encoder created within each process.

:option:`--threads` specifies the number of threads the encoder will
try to allocate for its thread pool.  If the thread pool was already
allocated this parameter is ignored.  By default x265 allocated one
thread per (hyperthreaded) CPU core in your system.

Work distribution is job based.  Idle worker threads ask their parent
pool object for jobs to perform.  When no jobs are available, idle
worker threads block and consume no CPU cycles.

Objects which desire to distribute work to worker threads are known as
job providers (and they derive from the JobProvider class).  When job
providers have work they enqueue themselves into the pool's provider
list (and dequeue themselves when they no longer have work).  The thread
pool has a method to **poke** awake a blocked idle thread, and job
providers are recommended to call this method when they make new jobs
available.

Worker jobs are not allowed to block except when abosultely necessary
for data locking.  If a job becomes blocked, the worker thread is
expected to drop that job and go back to the pool and find more work.

.. note::

	x265_cleanup() frees the process-global thread pool, allowing
	it to be reallocated if necessary, but only if no encoders are
	allocated at the time it is called.

Wavefront Parallel Processing
=============================

New with HEVC, Wavefront Parallel Processing allows each row of CTUs to
be encoded in parallel, so long as each row stays at least two CTUs
behind the row above it, to ensure the intra references and other data
of the blocks above and above-right are available. WPP has almost no
effect on the analysis and compression of each CTU and so it has a very
small impact on compression efficiency relative to slices or tiles. The
compression loss from WPP has been found to be less than 1% in most of
our tests.

WPP has three effects which can impact efficiency. The first is the row
starts must be signaled in the slice header, the second is each row must
be padded to an even byte in length, and the third is the state of the
entropy coder is transferred from the second CTU of each row to the
first CTU of the row below it.  In some conditions this transfer of
state actually improves compression since the above-right state may have
better locality than the end of the previous row.

Parabola Research have published an excellent HEVC
`animation <http://www.parabolaresearch.com/blog/2013-12-01-hevc-wavefront-animation.html>`_
which visualizes WPP very well.  It even correctly visualizes some of
WPPs key drawbacks, such as:

1. the low thread utilization at the start and end of each frame
2. a difficult block may stall the wave-front and it takes a while for
   the wave-front to recover.
3. 64x64 CTUs are big! there are much fewer rows than with H.264 and
   similar codecs

Because of these stall issues you rarely get the full parallelisation
benefit one would expect from row threading. 30% to 50% of the
theoretical perfect threading is typical.

In x265 WPP is enabled by default since it not only improves performance
at encode but it also makes it possible for the decoder to be threaded.

If WPP is disabled by :option:`--no-wpp` the frame will be encoded in
scan order and the entropy overheads will be avoided.  If frame
threading is not disabled, the encoder will change the default frame
thread count to be higher than if WPP was enabled.  The exact formulas
are described in the next section.


Frame Threading
===============

Frame threading is the act of encoding multiple frames at the same time.
It is a challenge because each frame will generally use one or more of
the previously encoded frames as motion references and those frames may
still be in the process of being encoded themselves.

Previous encoders such as x264 worked around this problem by limiting
the motion search region within these reference frames to just one
macroblock row below the coincident row being encoded. Thus a frame
could be encoded at the same time as its reference frames so long as it
stayed one row behind the encode progress of its references (glossing
over a few details). 

x265 has the same frame threading mechanism, but we generally have much
less frame parallelism to exploit than x264 because of the size of our
CTU rows. For instance, with 1080p video x264 has 68 16x16 macroblock
rows available each frame while x265 only has 17 64x64 CTU rows.

The second extenuating circumstance is the loop filters. The pixels used
for motion reference must be processed by the loop filters and the loop
filters cannot run until a full row has been encoded, and it must run a
full row behind the encode process so that the pixels below the row
being filtered are available. When you add up all the row lags each
frame ends up being 3 CTU rows behind its reference frames (the
equivalent of 12 macroblock rows for x264)

The third extenuating circumstance is that when a frame being encoded
becomes blocked by a reference frame row being available, that frame's
wave-front becomes completely stalled and when the row becomes available
again it can take quite some time for the wave to be restarted, if it
ever does. This makes WPP many times less effective when frame
parallelism is in use.

:option:`--merange` can have a negative impact on frame parallelism. If
the range is too large, more rows of CTU lag must be added to ensure
those pixels are available in the reference frames.  Similarly
:option:`--sao-lcu-opt` 0 will cause SAO to be performed over the
entire picture at once (rather than being CTU based), which prevents any
motion reference pixels from being available until the entire frame has
been encoded, which prevents any real frame parallelism at all.

.. note::

	Even though the merange is used to determine the amount of reference
	pixels that must be available in the reference frames, the actual
	motion search is not necessarily centered around the coincident
	block. The motion search is actually centered around the motion
	predictor, but the available pixel area (mvmin, mvmax) is determined
	by merange and the interpolation filter half-heights.

When frame threading is disabled, the entirety of all reference frames
are always fully available (by definition) and thus the available pixel
area is not restricted at all, and this can sometimes improve
compression efficiency. Because of this, the output of encodes with
frame parallelism disabled will not match the output of encodes with
frame parallelism enabled; but when enabled the number of frame threads
should have no effect on the output bitstream except when using ABR or
VBV rate control or noise reduction.

When :option:`--nr` is enabled, the outputs of each number of frame threads
will be deterministic but none of them will match becaue each frame
encoder maintains a cumulative noise reduction state.

VBV introduces non-determinism in the encoder, at this point in time,
regardless of the amount of frame parallelism.

By default frame parallelism and WPP are enabled together. The number of
frame threads used is auto-detected from the (hyperthreaded) CPU core
count, but may be manually specified via :option:`--frame-threads`

	+-------+--------+
	| Cores | Frames |
	+=======+========+
	|  > 32 |   6    |
	+-------+--------+
	| >= 16 |   5    |
	+-------+--------+
	| >= 8  |   3    |
	+-------+--------+
	| >= 4  |   2    |
	+-------+--------+

If WPP is disabled, then the frame thread count defaults to **min(cpuCount, ctuRows / 2)**

Over-allocating frame threads can be very counter-productive. They
each allocate a large amount of memory and because of the limited number
of CTU rows and the reference lag, you generally get limited benefit
from adding frame encoders beyond the auto-detected count, and often
the extra frame encoders reduce performance.

Given these considerations, you can understand why the faster presets
lower the max CTU size to 32x32 (making twice as many CTU rows available
for WPP and for finer grained frame parallelism) and reduce
:option:`--merange`

Each frame encoder runs in its own thread (allocated separately from the
worker pool). This frame thread has some pre-processing responsibilities
and some post-processing responsibilities for each frame, but it spends
the bulk of its time managing the wave-front processing by making CTU
rows available to the worker threads when their dependencies are
resolved.  The frame encoder threads spend nearly all of their time
blocked in one of 4 possible locations:

1. blocked, waiting for a frame to process
2. blocked on a reference frame, waiting for a CTU row of reconstructed
   and loop-filtered reference pixels to become available
3. blocked waiting for wave-front completion
4. blocked waiting for the main thread to consume an encoded frame

Lookahead
=========

The lookahead module of x265 (the lowres pre-encode which determines
scene cuts and slice types) uses the thread pool to distribute the
lowres cost analysis to worker threads. It follows the same wave-front
pattern as the main encoder except it works in reverse-scan order.

The function slicetypeDecide() itself may also be performed by a worker
thread if your system has enough CPU cores to make this a beneficial
trade-off, else it runs within the context of the thread which calls the
x265_encoder_encode().
