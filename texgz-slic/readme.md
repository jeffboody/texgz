SLIC Superpixels (Simple Linear Iterative Clustering)
=====================================================

This implementation includes several additional parameters
when compared with the initial SLIC Superpixel
implementation. The superpixel size s is used as an input
parameter rather than the cluster size k since the
superpixel size is independent of image size. A new sdx
threshold has been added to discard outlier samples within
a superpixel. The outlier detection uses correlation
(Z-Score) to determine which samples are outside a stddev
threshold. An additional flag has also been added to
restrict recentering the superpixel cluster after the
initialization operation to perterb the cluster centers.

Usage
-----

	usage: ./texgz-slic s m sdx n r steps prefix
	s: superpixel size (sxs)
	m: compactness control
	sdx: stddev threshold
	n: gradient neighborhood (nxn)
	r: recenter clusters
	steps: maximum step count

Analysis and Results
--------------------

I've selected three cases to compare results. All
experiments use 8x8 superpixels, a 3x3 gradient
neighborhood and 10 steps however the other
parameters vary. The experiments vary the compactness
control value, the stddev threshold and the recentering
clusters flag. A high compaction control value requires
the center to be movable as the superpixel shape will not
change significantly while a low compaction control value
allows the shape to change but does not require the center
to move. The stddev threshold makes it possible to identify
outlier samples in the cluster which can be discarded when
computing superpixel average values. My prefered values
based on an "eye test" are a compactness control of 1.0, a
stddev threshold of 1.0 while fixing the center location. I
still need to implement a method to evaluate the algorithm
parameters more robustly.

The following image shows the main output of the SLIC
algorithm which are the superpixel clusters.

![Compare Superpixels](data/tomato-256-compare1.png?raw=true "Compare Superpixels")

1. Top-Left: Original Image
2. Top-Right: SLIC with m=10.0, sdx=0 (disabled), r=1 (e.g. "defaults")
3. Bottom-Left: SLIC with m=1.0, sdx=1.0, r=0
4. Bottom-Right: SLIC with m=1.0, sdx=2.0, r=0

The following image shows the main output of the SLIC
algorithm which are the compressed superpixel clusters
when compared with an 8x8 averaging filter and an 8x8
inlier filter (see texgz-inlier).

![Compare Superpixels](data/tomato-256-compare4.png?raw=true "Compare Superpixels")

1. Top-Left: SLIC with m=10.0, sdx=0 (disabled), r=1 (e.g. "defaults")
2. Top-Center: Average of pixels with s=8
3. Top-Right: Inlier of pixels with s=8, sdx=1.0
4. Bottom-Left: SLIC with m=1.0, sdx=0.0, r=0
5. Bottom-Center: SLIC with m=1.0, sdx=1.0, r=0
6. Bottom-Right: SLIC with m=1.0, sdx=2.0, r=0

The following image shows the outliers identified by the
stddev threshold (sdx). Note that the red outlier pixels
lie predominately on the superpixel edge boundaries.

![Compare Outliers](data/tomato-256-compare2.png?raw=true "Compare Outliers")

1. Left: SLIC with m=1.0, sdx=1.0, r=0
2. Right: SLIC with m=1.0, sdx=2.0, r=0

The stddev of the samples for each superpixel was also
computed as an auxillary output which may be fed into
the classification algorithm.

![Compare StdDev](data/tomato-256-compare3.png?raw=true "Compare StdDev")

1. Top-Left: Gradient X (just for reference)
2. Top-Right: SLIC with m=10.0, sdx=0, r=1 (e.g. "defaults")
3. Bottom-Left: SLIC with m=1.0, sdx=1.0, r=0
4. Bottom-Right: SLIC with m=1.0, sdx=2.0, r=0

References
----------

* [SLIC Superpixels](https://www.iro.umontreal.ca/~mignotte/IFT6150/Articles/SLIC_Superpixels.pdf)
* [SLIC Superpixels Compared to State-of-the-art Superpixel Methods](https://core.ac.uk/download/pdf/147983593.pdf)
* [Superpixels and SLIC](https://darshita1405.medium.com/superpixels-and-slic-6b2d8a6e4f08)
* [SLIC - python](https://github.com/darshitajain/SLIC)
* [Random Forest and Objected-Based Classification for Forest Pest Extraction from UAV Aerial Imagery](https://www.researchgate.net/publication/303835823_RANDOM_FOREST_AND_OBJECTED-BASED_CLASSIFICATION_FOR_FOREST_PEST_EXTRACTION_FROM_UAV_AERIAL_IMAGERY)
* [Fast Segmentation and Classification of Very High Resolution Remote Sensing Data Using SLIC Superpixels](https://www.researchgate.net/publication/314492084_Fast_Segmentation_and_Classification_of_Very_High_Resolution_Remote_Sensing_Data_Using_SLIC_Superpixels)
* [Detecting and Removing Outliers](https://medium.com/analytics-vidhya/detecting-and-removing-outliers-7b408b279c9)
