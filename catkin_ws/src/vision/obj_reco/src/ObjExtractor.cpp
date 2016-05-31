#include "ObjExtractor.hpp"

bool ObjExtractor::DebugMode = false; 
bool ObjExtractor::UseBetterPlanes = false; 

std::vector<DetectedObject> ObjExtractor::GetObjectsInHorizontalPlanes(cv::Mat pointCloud)
{
	std::vector< DetectedObject > detectedObjectsList; 

	double ticks; 

	// PARAMS: Valid Points 
	double floorDistRemoval = 0.15;
	// PARAMS: Normals Extraction
	int blurSize = 5; 
	double normalZThreshold = 0.8; 
	// PARAMS: Planes RANSAC 
	double maxDistToPlane = 0.02; 
	int maxIterations = 1000; 
	int minPointsForPlane = pointCloud.rows*pointCloud.cols*0.05;
	// PARAMS: Object Extracction
	double minObjDistToPlane = maxDistToPlane; 
	double maxObjDistToPlane = 0.25; 
	double minDistToContour = 0.02; 
	double maxDistBetweenObjs = 0.05; 

	// For removing floor and far far away points 
	cv::Mat validPointCloud;
	cv::inRange(pointCloud, cv::Scalar(-3.0, -3.0, floorDistRemoval), cv::Scalar(3.0, 3.0, 3.0), validPointCloud); 
	
	// Getting Normals 	
	cv::Mat pointCloudBlur;
	cv::blur(pointCloud, pointCloudBlur, cv::Size(blurSize, blurSize));
	cv::Mat normals = ObjExtractor::CalculateNormals( pointCloudBlur ); 
	if(DebugMode)
		cv::imshow("normals", normals);

	// Getting Mask of Normals pointing horizonaliy
	cv::Mat  horizontalNormals;
	cv::inRange( normals, cv::Scalar(-1.0, -1.0, normalZThreshold), cv::Scalar(1.0,1.0, 1.0), horizontalNormals); 

	// Mask of horizontal normals and valid.  
	cv::Mat horizontalsValidPoints = horizontalNormals & validPointCloud; 

	
	//Getting horizontal planes.
	ticks = cv::getTickCount(); 
	std::vector<PlanarSegment> horizontalPlanes = ObjExtractor::ExtractHorizontalPlanesRANSAC(pointCloud, maxDistToPlane, maxIterations, minPointsForPlane, horizontalsValidPoints);
	if( DebugMode )
	{
		cv::Mat planesMat  = pointCloud.clone(); 
		for( int i=0; i<(int)horizontalPlanes.size(); i++)
		{
			std::vector< cv::Point2i > indexes = horizontalPlanes[i].Get_Indexes(); 
			cv::Vec3f color = ObjExtractor::RandomFloatColor();  
			for( int j=0; j<(int)indexes.size(); j++)
			{
				planesMat.at< cv::Vec3f >( indexes[j] ) = color; 
			}
		}
		cv::imshow("planesMat", planesMat); 
	}
	std::cout << "Getting horizontal planes t=" << ((double)cv::getTickCount() - ticks) / cv::getTickFrequency() << std::endl; 

	// Getting Mask of objects of every plane
	ticks = cv::getTickCount(); 
	std::vector< cv::Point3f > objectsPoints;
	std::vector< cv::Point2i > objectsIdx; 
	for( int i=0; i<(int)horizontalPlanes.size(); i++)
	{
		for(int row=0; row<pointCloud.rows; row++)
		{
			for(int col=0; col<pointCloud.cols; col++)
			{
				cv::Point3f xyzPoint = pointCloud.at< cv::Vec3f >(row, col ); 
				double dist = horizontalPlanes[i].Get_Plane().DistanceToPoint(xyzPoint, true);				

				if( minObjDistToPlane < dist && maxObjDistToPlane > dist )
				{
					double distToConvexHull = horizontalPlanes[i].IsInside( xyzPoint ); 
					if(distToConvexHull > minDistToContour)
					{
						objectsPoints.push_back( xyzPoint ); 
						objectsIdx.push_back( cv::Point2i(col, row) ); 
					}
				}
			}
		}
	}
	std::cout << " Getting Mask of objects of every plane t=" << ((double)cv::getTickCount() - ticks) / cv::getTickFrequency() << std::endl; 

	// Cluster objects by distance: 
	ticks = cv::getTickCount(); 
	std::vector< std::vector< int > > objIdxClusters  = ObjExtractor::SegmentByDistance( objectsPoints, maxDistBetweenObjs ); 
	
	cv::Mat objMat = cv::Mat::zeros(pointCloud.rows, pointCloud.cols, CV_8UC3); 
	for(int i=0; i<(int)objIdxClusters.size(); i++)
	{
		cv::Vec3b color( rand()%256, rand()%256, rand()%256);
		
		float maxZ = 0.0; 
		float minZ = 100000.0; 
		float objHeight	= 0.0; 
		cv::Point3f objCentroid( 0.0, 0.0, 0.0); 
	 	std::vector< cv::Point3f > objPoints3D; 
		std::vector< cv::Point2f > objPoints2D; 
		std::vector< cv::Point2i > objIndexes; 

		for( int j=0; j<(int)objIdxClusters[i].size(); j++)
		{
			cv::Point2i idx = objectsIdx[ objIdxClusters[i][j] ]; 
			cv::Point3f pnt = pointCloud.at< cv::Vec3f >( idx ); 
			// For Debug
			objMat.at<cv::Vec3b>( idx ) = color; 

			// Getting Object charactersics
			objCentroid += pnt; 
			
			if( pnt.z < minZ )
				minZ = pnt.z; 

			if( pnt.z > maxZ )
				maxZ = pnt.z; 
			
			objPoints3D.push_back( pnt );
			objPoints2D.push_back( cv::Point2f( pnt.x, pnt.y )) ; 
			objIndexes.push_back( idx ); 
		}

		objHeight = std::abs(maxZ - minZ); 
		objCentroid *= (1.0f / (float)objIdxClusters[i].size()); 
		
		DetectedObject detObj = DetectedObject( objIndexes, objPoints3D, objPoints2D, objHeight, objCentroid ); 
		detectedObjectsList.push_back( detObj ); 
	}
	std::cout << "Cluster objects by distance: t=" << ((double)cv::getTickCount() - ticks) / cv::getTickFrequency() << std::endl; 
	if(DebugMode)
	{
		cv::imshow( "objMat", objMat ); 	
	}

	
	return detectedObjectsList; 
}	

// Return labels corresponding to wich cluster belong each point 
std::vector< std::vector< int > > ObjExtractor::SegmentByDistance(std::vector< cv::Point3f > xyzPointsList, double distThreshold)
{
	std::vector< int > labels(xyzPointsList.size(), 0); 
	std::vector< std::vector< int > > labelsVec; 

	if( xyzPointsList.size() == 0 )
		return labelsVec; 

	cv::Mat xyzPointMat = cv::Mat(xyzPointsList).reshape(1);

	cv::flann::KDTreeIndexParams idxParams; 
	cv::flann::Index kdTree(xyzPointMat, idxParams); 

	// KD uses distance without squareRoot 
	double distSquare = distThreshold*distThreshold; 

	int labelCnt = 0;
	for(int p=0; p<xyzPointMat.rows; p++)
	{
		if( labels[p] != 0 )
			continue; 

		std::vector< int > labelCluster; 

		labelCnt++;   
		labels[p] = labelCnt; 

		std::queue< cv::Mat > nnQueue; 	
		nnQueue.push( xyzPointMat.row(p) ); 

		labelCluster.push_back( p ); 

		while( nnQueue.size() > 0 )
		{
			cv::Mat currentPoint = nnQueue.front(); 
			nnQueue.pop(); 

			std::vector<float> dists(1000); 
			std::vector<int> idxs(1000); 
			kdTree.radiusSearch( currentPoint, idxs, dists, distSquare, 64 ); 

			for(int i=0; i<(int)idxs.size(); i++)
			{
				if( labels[ idxs[i] ] == 0)
				{
					labels[ idxs[i] ] = labelCnt; 
					nnQueue.push( xyzPointMat.row( idxs[i]) ); 
					labelCluster.push_back( idxs[i] );
				}
			}
		}
		labelsVec.push_back( labelCluster ); 
	}
	return labelsVec; 
}

std::vector< std::vector< cv::Point2i > > ObjExtractor::SegmentByDistanceMat(cv::Mat pointCloud, cv::Mat mask, double distThreshold)
{
	std::vector< cv::Point3f > xyzPointsList; 
	std::vector< cv::Point2i > xyzPixelsList; 
	for(int row=0; row<pointCloud.rows; row++)
	{
		for(int col=0; col<pointCloud.cols; col++)
		{
			if( mask.at<uchar>(row,col) == 0 )
				continue; 
			xyzPointsList.push_back( pointCloud.at< cv::Vec3f >( row, col ) ); 
			xyzPixelsList.push_back( cv::Point2i( col, row ) ); 
		}
	}

	std::vector< std::vector<int> > segmentedIndexes = ObjExtractor::SegmentByDistance( xyzPointsList, distThreshold); 

	std::vector< std::vector<cv::Point2i> > segmentedPixels; 
	for( int i=0; i<(int)segmentedIndexes.size(); i++)
	{
		std::vector< cv::Point2i > segPixels; 
		for( int j=0; j<(int)segmentedIndexes[i].size(); j++)
		{
			segPixels.push_back( xyzPixelsList[ segmentedIndexes[i][j] ] ); 
		}
		segmentedPixels.push_back( segPixels ); 
	}
	return segmentedPixels; 
}

std::vector<PlanarSegment> ObjExtractor::ExtractHorizontalPlanesRANSAC(cv::Mat pointCloud, double maxDistPointToPlane, int maxIterations, int minPointsForPlane, cv::Mat mask)
{
	std::vector< PlanarSegment > horizontalPlanesList; 

	// Getting mask indexe
	std::vector< cv::Point2i > indexes; 
	for( int i=0; i<mask.rows; i++)
	{
		for(int j=0; j< mask.cols; j++) 
		{
			if( mask.at<uchar>(i,j) != 0 )
				indexes.push_back( cv::Point(j,i) ); 
		}
	}

	int cntIteratiosn = 0; 
	while( cntIteratiosn++ < maxIterations && indexes.size() > minPointsForPlane)
	{
		cv::Point3f p1  = pointCloud.at< cv::Vec3f >( indexes[rand()%indexes.size()] ); 
		cv::Point3f p2  = pointCloud.at< cv::Vec3f >( indexes[rand()%indexes.size()] ); 
		cv::Point3f p3  = pointCloud.at< cv::Vec3f >( indexes[rand()%indexes.size()] ); 

		if( !Plane3D::AreValidPointsForPlane( p1, p2, p3))
			continue; 

		Plane3D candidatePlane( p1, p2, p3);

		// Heuristics ??
		if( std::abs(candidatePlane.GetNormal().z < 0.99) )
			continue; 

		// For all points, checking distance to candidate plane (getting inliers) 
		std::vector< cv::Point3f > inliers; 
		for( int i=0; i < (int)indexes.size(); i++)
		{
			cv::Point3f xyzPoint = pointCloud.at<cv::Vec3f>(indexes[i]);
			double distToPlane = candidatePlane.DistanceToPoint(xyzPoint);

			if( distToPlane < maxDistPointToPlane )
				inliers.push_back(xyzPoint); 
		}

		if( inliers.size() < minPointsForPlane )
			continue; 

		// Getting better plane using PCA
		cv::PCA pca( cv::Mat(inliers).reshape(1), cv::Mat(), CV_PCA_DATA_AS_ROW); 
		cv::Point3f pcaNormal( pca.eigenvectors.at<float>(2,0), pca.eigenvectors.at<float>(2,1), pca.eigenvectors.at<float>(2,2) ); 
		cv::Point3f pcaPoint(pca.mean.at<float>(0,0), pca.mean.at<float>(0,1), pca.mean.at<float>(0,2));

		Plane3D refinedPlane( pcaNormal, pcaPoint ); 

		// Obtaining Refined Plane
		std::vector< cv::Point3f > pointsPlane;
		std::vector< cv::Point2f > xyPointsPlane; 
		std::vector< cv::Point2i > indexesPlane;
		std::vector< cv::Point2i > indexesNew; 

		for( int i=0; i<(int)indexes.size() ; i++)
		{
			cv::Point3f xyzPoint = pointCloud.at<cv::Vec3f>(indexes[i]);
			double distToPlane = refinedPlane.DistanceToPoint(xyzPoint);

			if( distToPlane < maxDistPointToPlane )
			{
				pointsPlane.push_back( xyzPoint );
				xyPointsPlane.push_back( cv::Point2f(xyzPoint.x, xyzPoint.y) ); 
				indexesPlane.push_back( indexes[i] ); 
			}
			else
			{
				indexesNew.push_back( indexes[i] ); 
			}
		}
		indexes = indexesNew; 

		if( ObjExtractor::UseBetterPlanes )
		{
			std::cout << "Use better Planes" << std::endl; 
			// Segmenting plane
			std::vector< std::vector<int> > clusterPlane = SegmentByDistance( pointsPlane, 0.10 ); 
			int maxSize = 0; 
			int maxIndex = 0; 
			for(int i=0; i<clusterPlane.size(); i++)
			{
				if( clusterPlane[i].size() > maxSize) 
				{
					maxSize = clusterPlane[i].size(); 
					maxIndex = i; 
				}
			}

			// Return points if cluster are small 
			if( maxSize < minPointsForPlane )
			{
				indexes.insert( indexes.end(), indexesPlane.begin(), indexesPlane.end() ); 
				continue; 
			}

			// Remove only bigger cluster. 
			std::vector< cv::Point3f > finalPointsPlane;
			std::vector< cv::Point2i > finalIndexesPlane;
			std::vector< cv::Point2f > finalXYPointsPlane; 
			for( int i=0; i<clusterPlane.size(); i++)
			{
				for( int j=0; j<clusterPlane[i].size() ; j++)
				{
					if( i == maxIndex ) 
					{
						cv::Point3f ptPlane = pointsPlane[ clusterPlane[i][j] ];  

						finalPointsPlane.push_back(ptPlane); 
						finalIndexesPlane.push_back( indexesPlane[ clusterPlane[i][j] ] ); 
						finalXYPointsPlane.push_back(cv::Point2f(ptPlane.x, ptPlane.y)); 
					}
					else
					{
						indexes.push_back( indexesPlane[ clusterPlane[i][j] ] ) ; 
					}
				}
			}
			pointsPlane = finalPointsPlane ;
			indexesPlane = finalIndexesPlane;
			xyPointsPlane = finalXYPointsPlane; 
		}

		// GETTING REFINED PLANES 
		cv::PCA pca2( cv::Mat(pointsPlane).reshape(1), cv::Mat(), CV_PCA_DATA_AS_ROW); 
		cv::Point3f pcaNormal2( pca2.eigenvectors.at<float>(2,0), pca2.eigenvectors.at<float>(2,1), pca2.eigenvectors.at<float>(2,2) ); 
		cv::Point3f pcaPoint2( pca2.mean.at<float>(0,0), pca2.mean.at<float>(0,1), pca2.mean.at<float>(0,2));


		// Getting Convex Hull ( Convex hull is valid if remove Z, because is an horizontal plane ) 
		std::vector< cv::Point2f > convexHull2D; 
		cv::convexHull(xyPointsPlane , convexHull2D);	

		PlanarSegment ps = PlanarSegment( Plane3D(pcaNormal2, pcaPoint2), pointsPlane, indexesPlane, convexHull2D); 
		horizontalPlanesList.push_back( ps ); 
	}

	return horizontalPlanesList; 
}

cv::Mat ObjExtractor::CalculateNormals(cv::Mat pointCloud, cv::Mat mask)
{
	cv::Mat normals = cv::Mat::zeros(pointCloud.rows, pointCloud.cols, CV_32FC3); 

	if( !mask.data )
		mask = cv::Mat::ones(pointCloud.rows, pointCloud.cols, CV_8UC1); 

	cv::Point3f pointi; 
	cv::Point3f topLeft; 
	cv::Point3f topRight; 
	cv::Point3f downLeft; 
	cv::Point3f downRight; 

	cv::Point3f normal_1; 
	cv::Point3f normal_2; 
	cv::Point3f normal; 
			
	cv::Point3f viewPoint(0.0,0.0,0.0);

	for( int idxRow = 1 ; idxRow < pointCloud.rows - 1 ; idxRow++ )
	{
		for( int idxCol = 1 ; idxCol < pointCloud.cols - 1 ; idxCol++ )
		{			
			//if( mask.at<uchar>( idxRow,idxCol ) == 0.0 )
				//continue; 
			
			// Getting Vectors
			pointi = pointCloud.at<cv::Vec3f>(idxRow, idxCol);

			topLeft = pointCloud.at<cv::Vec3f>(idxRow-1, idxCol-1);
			topRight = pointCloud.at<cv::Vec3f>(idxRow-1, idxCol+1);
			downLeft = pointCloud.at<cv::Vec3f>(idxRow+1, idxCol-1);
			downRight = pointCloud.at<cv::Vec3f>(idxRow+1, idxCol+1);
			
			if( topLeft.x == 0.0 && topLeft.y == 0.0 && topLeft.z == 0.0 )
				continue; 
			if( topRight.x == 0.0 && topRight.y == 0.0 && topRight.z == 0.0 )
				continue; 
			if( downLeft.x == 0.0 && downLeft.y == 0.0 && downLeft.z == 0.0 )
				continue; 
			if( downRight.x == 0.0 && downRight.y == 0.0 && downRight.z == 0.0 )
				continue; 
			
			// Normals
			normal_1 = topRight - downLeft;
			normal_2 = topLeft - downRight; 

			// Normal by cross product (v x h)
			normal  = normal_2.cross(normal_1);

			// Make normal unitary and assignin to mat
			float norm = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
			if(norm != 0.0f)
			{
				// Proyecting all normals over XY plane
				//if(normal.dot(viewPoint-pointi)<0)
				//	normal *= -1;
				if( normal.z < 0 )
					normal *= -1; 

				normals.at<cv::Vec3f>(idxRow, idxCol) = ( 1.0f / norm ) * normal;
			}
		}
	}
	return normals;
}

cv::Vec3f ObjExtractor::RandomFloatColor()
{
	float x = (float)(rand()) / (float)(RAND_MAX);  
	float y = (float)(rand()) / (float)(RAND_MAX);  
	float z = (float)(rand()) / (float)(RAND_MAX);  

	return cv::Vec3f(x,y,z);  
}

std::vector<PlanarSegment> ObjExtractor::ExtractHorizontalPlanesRANSAC_2(cv::Mat pointCloud, double maxDistPointToPlane, int maxIterations, int minPointsForPlane, cv::Mat mask)
{
 	std::cout << "NOT IMPLEMETNED YET !!!" << std::endl; 

	std::vector< PlanarSegment > horizontalPlanesList; 
	return horizontalPlanesList; 
	
	// Getting mask indexe
	std::vector< cv::Point2i > pixelsTotalVec; 
	std::vector< cv::Point3f > pointsTotalVec;
	std::vector< int > remainingPointsIndexes; 
	int cnt = 0; 
	for( int i=0; i<mask.rows; i++)
	{
		for(int j=0; j< mask.cols; j++) 
		{
			if( mask.at<uchar>(i,j) != 0 )
			{
				pixelsTotalVec.push_back( cv::Point(j,i) ); 
				pointsTotalVec.push_back( pointCloud.at< cv::Vec3f >(i,j)); 
				remainingPointsIndexes.push_back( cnt++ ); 
			}
		}
	}
	cv::Mat pixelsTotalMat = cv::Mat(pointsTotalVec).reshape(1);

	//Creating KDTREE
	cv::flann::KDTreeIndexParams idxParams; 
	cv::flann::Index kdTree(pixelsTotalMat, idxParams); 

	int initRadius = 0.05; 

	//RANSAC (maybe ?)
	int cntIteratiosn = 0; 
	while( cntIteratiosn++ < maxIterations && remainingPointsIndexes.size() > minPointsForPlane)
	{
		//Getting candidate plane using pca and NN
		
		cv::Mat randPoint = pixelsTotalMat.row( remainingPointsIndexes[ rand() % remainingPointsIndexes.size() ] ); 
		
		std::vector<float> tempDists(1000); 
		std::vector<int> tempIdxs(1000); 
		kdTree.radiusSearch( randPoint, tempIdxs, tempDists, initRadius*initRadius, 16 ); 

		std::vector< cv::Point3f > tempNearestNeighs(1000); 
		for( int i=0; i<tempIdxs.size(); i++)
			tempNearestNeighs.push_back( pointsTotalVec[ tempIdxs[i]  ] );

		cv::PCA pca( cv::Mat(tempNearestNeighs).reshape(1), cv::Mat(), CV_PCA_DATA_AS_ROW); 
		cv::Point3f pcaNormal( pca.eigenvectors.at<float>(2,0), pca.eigenvectors.at<float>(2,1), pca.eigenvectors.at<float>(2,2) ); 
		cv::Point3f pcaPoint(pca.mean.at<float>(0,0), pca.mean.at<float>(0,1), pca.mean.at<float>(0,2));
		
		Plane3D candidatePlane( pcaNormal, pcaPoint ); 

		// Heuristics
		if( std::abs(candidatePlane.GetNormal().z < 0.99) )
			continue; 
		std::cout << "Candidate Plane ! i=" << cntIteratiosn << std:: endl; 

		// Getting Neighbours of 
		
		
	}
}