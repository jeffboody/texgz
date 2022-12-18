SLIC Superpixels
================

Simple Linear Iterative Clustering (SLIC)

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

	usage: ./texgz-slic s m sdx n r steps prefix
	s: superpixel size (sxs)
	m: compactness control
	sdx: stddev threshold
	n: gradient neighborhood (nxn)
	r: recenter clusters
	steps: maximum step count

References
==========

![SLIC Superpixels](https://www.iro.umontreal.ca/~mignotte/IFT6150/Articles/SLIC_Superpixels.pdf)
![SLIC Superpixels Compared to State-of-the-art Superpixel Methods](https://core.ac.uk/download/pdf/147983593.pdf)
![Superpixels and SLIC](https://darshita1405.medium.com/superpixels-and-slic-6b2d8a6e4f08)
![SLIC - python](https://github.com/darshitajain/SLIC)
![Random Forest and Objected-Based Classification for Forest Pest Extraction from UAV Aerial Imagery](https://www.researchgate.net/publication/303835823_RANDOM_FOREST_AND_OBJECTED-BASED_CLASSIFICATION_FOR_FOREST_PEST_EXTRACTION_FROM_UAV_AERIAL_IMAGERY)
![Fast Segmentation and Classification of Very High Resolution Remote Sensing Data Using SLIC Superpixels](https://www.researchgate.net/publication/314492084_Fast_Segmentation_and_Classification_of_Very_High_Resolution_Remote_Sensing_Data_Using_SLIC_Superpixels)
![Detecting and Removing Outliers](https://medium.com/analytics-vidhya/detecting-and-removing-outliers-7b408b279c9)
