// Author: Xinshuo
// Email: xinshuow@andrew.cmu.edu

// pcl library
#include <pcl/conversions.h>
#include <pcl/io/auto_io.h>


// self_contained library
#include <computer_vision/geometry/mycamera.h>
#include <computer_vision/geometry/MyMesh.h>
#include <computer_vision/geometry/pts_on_mesh.h>
#include <computer_vision/geometry/camera_geometry.h>
#include <miscellaneous/debug_tool.h>
#include <miscellaneous/type_conversion.h>
#include <math/math_functions.h>

const double projection_err_threshold = 8;

MyMesh::MyMesh(char* filename, int scale) {
	pcl::PolygonMesh::Ptr poly_ptr_tmp(new pcl::PolygonMesh);
	poly_ptr = poly_ptr_tmp;

    clock_t old_time = clock();
	pcl::io::load(filename, *poly_ptr_tmp.get());
	cloud = pcl::PointCloud<pcl::PointXYZ>::Ptr(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::fromPCLPointCloud2(poly_ptr->cloud, *cloud.get());
	for (unsigned int i = 0; i < cloud->size(); i++) {
		cloud->points[i].x *= scale;		//rescale the point to the original scale
		cloud->points[i].y *= scale;
		cloud->points[i].z *= scale;
	}

    clock_t new_time = clock();
    std::cout << "It spend " << float(new_time - old_time) / CLOCKS_PER_SEC << " to load" << std::endl;
    old_time = new_time;

	int num_vertices = cloud->size();
	int num_planes = poly_ptr_tmp->polygons.size();
    std::cout << "number of planes in the loaded mesh is " << num_planes << std::endl;
    std::cout << "number of vertices in the loaded mesh is " << num_vertices << std::endl;
	planes_of_vertices.resize(num_vertices);
	first_plane_of_vertices.resize(num_vertices);
	planes.resize(num_planes);

	for (int i = 0; i < num_planes; i++) {
//		ASSERT_WITH_MSG(poly_ptr_tmp->polygons[i].vertices.size() == 3, "The vertices of mesh should have 3 coordinates");			// ensure mesh is in 3d

        int size_polygon = poly_ptr_tmp->polygons[i].vertices.size();
        std::vector<int> idxs(size_polygon);
//        std::cout << "polygon size is " <<  << std::endl;
		for (int j = 0; j < size_polygon; j++) {
			idxs[j] = poly_ptr_tmp->polygons[i].vertices[j];			// find the id of vertices corresponding to all polygon (triangle)
		}
		//std::cout << "idxs is:" << std::endl;
		//print_vec(idxs);

		plane_pts_idx.push_back(idxs);							// idxs is three points id for current plane, plane_pts_idx store the vertices id for all planes

//		Eigen::Matrix3f A, B;									// A stores 3 3d points for each polygon
//		A << cloud->points[idxs[0]].x, cloud->points[idxs[0]].y, cloud->points[idxs[0]].z,
//			cloud->points[idxs[1]].x, cloud->points[idxs[1]].y, cloud->points[idxs[1]].z,
//			cloud->points[idxs[2]].x, cloud->points[idxs[2]].y, cloud->points[idxs[2]].z;		// cloud stores the exact 3d coordiante indexed by vertices id
//		B << 0, 0, 0, 1, 0, 0, 0, 1, 0;
//		Eigen::ColPivHouseholderQR<Eigen::Matrix3f> dec(A);
//		plane_projection.push_back(dec.solve(B));			// keep
//		if (dec.rank() != 3) {
//			plane_projection_good.push_back(false);
//		}
//		else {
//			plane_projection_good.push_back(true);
//		}

		double normal_length = get_3d_plane(cloud->points[idxs[0]], cloud->points[idxs[1]], cloud->points[idxs[2]], planes[i]);		// planes is the 4d vector representing that plane
		if (normal_length > EPS_SMALL) {			// if the length is very small, then the three points forming that plane is in the same straight line
			first_plane_of_vertices[idxs[0]] = i;
			for (int j = 0; j < 3; j++) {
				planes_of_vertices[idxs[j]].push_back(i);		// planes of vertices stores which plane this points belongs to indexed by points id
			}
		}
	}

    new_time = clock();
    std::cout << "It spend " << float(new_time - old_time) / CLOCKS_PER_SEC << " to process" << std::endl;
	old_time = new_time;
}


MyMesh::~MyMesh() {
}

pts_on_mesh* MyMesh::pts_back_projection_single_view(pts_2d_conf& pts_2d, mycamera& camera_src, const bool consider_dist) {
	std::vector<double> ray;
	cv::Point3d C;

	get_3d_ray(pts_2d, camera_src, C, ray, consider_dist);
	return this->get_pts_on_mesh(C, ray, pts_2d.conf);

}

pts_on_mesh* MyMesh::pts_back_projection_single_view(pts_2d_conf& pts_2d, pts_2d_conf& pts_2d_ref, mycamera& camera_src, mycamera& camera_ref, const bool consider_dist) {
    std::vector<double> ray;
    cv::Point3d C, pts_3d_ref;

    std::vector<pts_2d_conf> vec_pts_2d, vec_pts_2d_ref;
    std::vector<pts_3d_conf> vec_pts_3d;
    vec_pts_2d.push_back(pts_2d);
    vec_pts_2d_ref.push_back(pts_2d_ref);

    triangulation_from_two_views(vec_pts_2d, vec_pts_2d_ref, camera_src, camera_ref, vec_pts_3d, consider_dist);
    std::cout << "the reference 3d point is ";
    vec_pts_3d[0].print();

    get_3d_ray(pts_2d, camera_src, C, ray, consider_dist);
    return this->get_pts_on_mesh_heuristic(C, ray, pts_2d.conf, vec_pts_3d[0].convert_to_point3d());
}

// TODO: test for correctness
pts_on_mesh* MyMesh::pts_back_projection_multiview(std::map<std::string, pts_2d_conf>& pts_2d, std::vector<mycamera>& camera_cluster, const bool consider_dist) {
	ASSERT_WITH_MSG(consider_dist == 0, "Back projection doesn't support undistortion right now. Please undistort first!");
	ASSERT_WITH_MSG(pts_2d.size() == camera_cluster.size(), "The size of the input points and camera is not equal.");
	double error_minimum = -1;
	int minimum_camera_id = -1;
	double error_tmp;
	for (int i = 0; i < camera_cluster.size(); i++) {
		pts_2d_conf pts_tmp = pts_2d[camera_cluster[i].name];
		//pts_tmp.print();
		pts_on_mesh* pts_3d = pts_back_projection_single_view(pts_tmp, camera_cluster[i], consider_dist);	// obtain 3d point on the mesh from single view
		//pts_3d->print();

		// calculate error
		cv::Point3d pts_src(pts_3d->x, pts_3d->y, pts_3d->z);
		//error_tmp = calculate_projection_error(pts_src, camera_cluster, pts_2d, true, consider_dist);
		error_tmp = calculate_projection_error(pts_src, camera_cluster, pts_2d, true);

		if (error_minimum == -1 || error_tmp < error_minimum) {
			error_minimum = error_tmp;
			minimum_camera_id = i;
		}
	}

	return pts_back_projection_single_view(pts_2d[camera_cluster[minimum_camera_id].name], camera_cluster[minimum_camera_id], consider_dist);
}

// TODO: test for correctness
std::vector<pts_on_mesh*> MyMesh::pts_back_projection_multiview(std::vector<std::map<std::string, pts_2d_conf>>& pts_2d, std::vector<mycamera>& camera_cluster, const bool consider_dist) {
	std::cout << "starting back projection for multiple points from multiview camera" << std::endl << std::endl;
	ASSERT_WITH_MSG(consider_dist == false, "Back projection doesn't support undistortion right now. Please undistort first!");
	std::vector<pts_on_mesh*> pts_dst;
	//std::cout << pts_2d.size() << std::endl;
	for (int i = 0; i < pts_2d.size(); i++) {
		std::cout << "working on view " << i << ", ";
		ASSERT_WITH_MSG(pts_2d[i].size() == camera_cluster.size(), "The size of the input points and camera is not equal.");
		pts_dst.push_back(pts_back_projection_multiview(pts_2d[i], camera_cluster, consider_dist));
	}
	return pts_dst;
}

pts_on_mesh* MyMesh::get_pts_on_mesh(cv::Point3d C_src, std::vector<double>& ray, double conf) {
	std::cout << "finding the closest point on the mesh given a 3d point." << std::endl;
	clock_t old_time = clock();
	double closest = -1;
	std::vector<double> cpts;
	int tri_id = -1;
	int planes = this->planes.size();
	std::vector<double> C = cv2vec_pts3d(C_src);

	// go though all planes
	float time_accu1 = 0;
	float time_accu2 = 0;
	for (int plane_index = 0; plane_index < planes; plane_index++) {


		// plane is plane[0] * x + plane[1] * y + plane[2] * z + plane[3] = 0
		// expected 3d point is [C[0] + t * ray[0], C[1] + t * ray[1], C[2] + t * ray[2]], t is the parameter we need to find
		std::vector<double>& plane = this->planes[plane_index];
		double n = 0, d = 0;
		for (int l = 0; l < 3; l++) {
			n += C[l] * plane[l];
			d += ray[l] * plane[l];
		}
		n += plane[3];
        double t = -n / d;


		clock_t new_time = clock();
		time_accu1 += float(new_time - old_time);
		old_time = new_time;

//        std::cout << "n is " << n << std::endl;
//        std::cout << "d is " << d << std::endl;

		if (std::abs(d) < EPS_SMALL) {
//            std::cout << "Skipped current plane for unknown reason 1 ...\n" << std::endl;
            continue;
        }

		// t should be larger than 0, otherwise the 3d point is in the wrong direction oppose to the ray
		if (t < 0) {
//			std::cout << "Skipped current plane because the direction is the opposite  ...\n" << std::endl;
			continue;
		}

		// even though t is larger than 0, is further than a 3d point found before
		if (closest != -1 && t >= closest) {
//			std::cout << "Skipped current plane because the it is further than the 3d point found before..." << std::endl;
			continue;
		}

        std::vector<double> pts_tmp;
		for (int i = 0; i < 3; i++) {
			pts_tmp.push_back(t * ray[i] + C[i]);	// the 3d point
		}

		std::vector<double> pts_append = pts_tmp;
		pts_append.push_back(1.0);

//		std::cout << "checking the point on triangle...\n" << std::endl;
//		ASSERT_WITH_MSG(std::abs(inner(pts_append, plane)) < EPS_MYSELF, "Point found is not even on the extended triangle plane. The offset is " + std::to_string(std::abs(inner(pts_append, plane))));		// check the point with parameter t is inside the current plane

		std::vector<int> vec_id = this->plane_pts_idx[plane_index];
		std::vector<double> tri_a, tri_b, tri_c;
		tri_a.push_back(this->cloud->points[vec_id[0]].x);
		tri_a.push_back(this->cloud->points[vec_id[0]].y);
		tri_a.push_back(this->cloud->points[vec_id[0]].z);
		tri_b.push_back(this->cloud->points[vec_id[1]].x);
		tri_b.push_back(this->cloud->points[vec_id[1]].y);
		tri_b.push_back(this->cloud->points[vec_id[1]].z);
		tri_c.push_back(this->cloud->points[vec_id[2]].x);
		tri_c.push_back(this->cloud->points[vec_id[2]].y);
		tri_c.push_back(this->cloud->points[vec_id[2]].z);

		// check if the point is inside the triangle
//		std::cout << "checking the triangle...\n" << std::endl;
		if (point_triangle_test_3d(pts_tmp, tri_a, tri_b, tri_c)) {
			if (tri_id == -1 || t < closest) {		// find the exact right plane which intersect with the ray and the intersection point is inside the plane.
//                std::cout << "t is " << t << std::endl;
                closest = t;
				tri_id = plane_index;
				cpts = pts_tmp;
			}
		}

		new_time = clock();
		time_accu2 += float(new_time - old_time);
		old_time = new_time;
	}

//	std::cout << "It spend " << time_accu1 / CLOCKS_PER_SEC << " to calculate the line parameters" << std::endl;
//	std::cout << "It spend " << time_accu2 / CLOCKS_PER_SEC << " to check" << std::endl;

//	std::cout << "plane id is:" << tri_id << std::endl;
//	std::cout << std::endl;
//	std::cout << "plane is:" << std::endl;
//	print_vec(this->planes[tri_id]);
//	std::cout << std::endl;
//
//	std::cout << "id of 3 points around the plane are" << std::endl;
//	std::vector<int> ids = this->plane_pts_idx[tri_id];
//	std::cout << ids[0] << ", " << ids[1] << ", " << ids[2] << std::endl;
//	std::cout << std::endl;

	//std::cout << "The 3d coordinate of three points in the intersecting plane are:" << std::endl;
	//std::vector<double> pts1, pts2, pts3;
	//pts1.push_back(this->cloud->points[ids[0]].x);
	//pts1.push_back(this->cloud->points[ids[0]].y);
	//pts1.push_back(this->cloud->points[ids[0]].z);
	//pts2.push_back(this->cloud->points[ids[1]].x);
	//pts2.push_back(this->cloud->points[ids[1]].y);
	//pts2.push_back(this->cloud->points[ids[1]].z);
	//pts3.push_back(this->cloud->points[ids[2]].x);
	//pts3.push_back(this->cloud->points[ids[2]].y);
	//pts3.push_back(this->cloud->points[ids[2]].z);
	//print_vec(pts1);
	//print_vec(pts2);
	//print_vec(pts3);
	//std::cout << std::endl;

	//std::cout << "t is:" << t << std::endl;
	//std::cout << std::endl;

	//print_vec(cpts);
	//std::cout << std::endl;

	pts_on_mesh* ptr_mesh;
	if (tri_id == -1) {
		conf = 0;
		fprintf(stderr, "no projected triangles!\n");

		ptr_mesh = new pts_on_mesh(-1, -1, 0.0, 0.0, 0.0, conf);
		return ptr_mesh;
	}

	//ptr_mesh = new pts_on_mesh(this->plane_pts_idx[tri_id][0], cpts[0], cpts[1], cpts[2], conf);
	pts_3d_conf pts_3d(cpts[0], cpts[1], cpts[2], conf);
	//std::cout << "final 3d points is" << std::endl;
	//pts_3d.print();
	pts_on_mesh* pts_mesh = find_closest_pts_on_mesh(pts_3d, tri_id);
	pts_mesh->print();

	return pts_mesh;
}


// note that we add heuristic to this function, the furtherest point (within the error range) should be the right choice
pts_on_mesh* MyMesh::get_pts_on_mesh(cv::Point3d C_src, std::vector<double>& ray, double conf, cv::Point3d pts_3d_ref) {
    std::cout << "finding the closest point on the mesh given a 3d point." << std::endl;
    clock_t old_time = clock();
    double closest = 100000;
    std::vector<double> pts_3d_ref_vec = cv2vec_pts3d(pts_3d_ref);
    std::vector<double> cpts;
    int tri_id = -1;
    int planes = this->planes.size();
    std::vector<double> C = cv2vec_pts3d(C_src);

    // go though all planes
    float time_accu1 = 0;
    float time_accu2 = 0;
    for (int plane_index = 0; plane_index < planes; plane_index++) {


        // plane is plane[0] * x + plane[1] * y + plane[2] * z + plane[3] = 0
        // expected 3d point is [C[0] + t * ray[0], C[1] + t * ray[1], C[2] + t * ray[2]], t is the parameter we need to find
        std::vector<double>& plane = this->planes[plane_index];
        double n = 0, d = 0;
        for (int l = 0; l < 3; l++) {
            n += C[l] * plane[l];
            d += ray[l] * plane[l];
        }
        n += plane[3];
        double t = -n / d;


        clock_t new_time = clock();
        time_accu1 += float(new_time - old_time);
        old_time = new_time;

//        std::cout << "n is " << n << std::endl;
//        std::cout << "d is " << d << std::endl;

        if (std::abs(d) < EPS_SMALL) {
//            std::cout << "Skipped current plane for unknown reason 1 ...\n" << std::endl;
            continue;
        }

        // t should be larger than 0, otherwise the 3d point is in the wrong direction oppose to the ray
        if (t < 0) {
//			std::cout << "Skipped current plane because the direction is the opposite  ...\n" << std::endl;
            continue;
        }

        std::vector<double> pts_tmp;
        for (int i = 0; i < 3; i++) {
            pts_tmp.push_back(t * ray[i] + C[i]);	// the 3d point
        }

        // even though t is larger than 0, is further than a 3d point found before
        double dist = compute_distance(pts_tmp, pts_3d_ref_vec);                   // compute the distance between the found point and reference 3d point
        if (dist >= closest || dist >= projection_err_threshold) {
//			std::cout << "Skipped current plane because the it is further than the 3d point found before..." << std::endl;
            continue;
        }

//        std::vector<double> pts_append = pts_tmp;
//        pts_append.push_back(1.0);

//		std::cout << "checking the point on triangle...\n" << std::endl;
//		ASSERT_WITH_MSG(std::abs(inner(pts_append, plane)) < EPS_MYSELF, "Point found is not even on the extended triangle plane. The offset is " + std::to_string(std::abs(inner(pts_append, plane))));		// check the point with parameter t is inside the current plane

        std::vector<int> vec_id = this->plane_pts_idx[plane_index];
        std::vector<double> tri_a, tri_b, tri_c;
        tri_a.push_back(this->cloud->points[vec_id[0]].x);
        tri_a.push_back(this->cloud->points[vec_id[0]].y);
        tri_a.push_back(this->cloud->points[vec_id[0]].z);
        tri_b.push_back(this->cloud->points[vec_id[1]].x);
        tri_b.push_back(this->cloud->points[vec_id[1]].y);
        tri_b.push_back(this->cloud->points[vec_id[1]].z);
        tri_c.push_back(this->cloud->points[vec_id[2]].x);
        tri_c.push_back(this->cloud->points[vec_id[2]].y);
        tri_c.push_back(this->cloud->points[vec_id[2]].z);

        // check if the point is inside the triangle
//		std::cout << "checking the triangle...\n" << std::endl;
        if (point_triangle_test_3d(pts_tmp, tri_a, tri_b, tri_c)) {
            std::cout << "t is " << t << ", current point found is " << pts_tmp[0] << ", " << pts_tmp[1] << ", " << pts_tmp[2] << ", distance is " << dist << ", previous closest distance is " << closest << std::endl;
            if (tri_id == -1 || dist < closest) {		// find the exact right plane which intersect with the ray and the intersection point is inside the plane.
//                std::cout << "t is " << t << std::endl;
//                closest = t;
                closest = dist;
                std::cout << "current closest distance is " << closest << std::endl;
                tri_id = plane_index;
                cpts = pts_tmp;
            }
        }

        new_time = clock();
        time_accu2 += float(new_time - old_time);
        old_time = new_time;
    }

//	std::cout << "It spend " << time_accu1 / CLOCKS_PER_SEC << " to calculate the line parameters" << std::endl;
//	std::cout << "It spend " << time_accu2 / CLOCKS_PER_SEC << " to check" << std::endl;

//	std::cout << "plane id is:" << tri_id << std::endl;
//	std::cout << std::endl;
//	std::cout << "plane is:" << std::endl;
//	print_vec(this->planes[tri_id]);
//	std::cout << std::endl;
//
//	std::cout << "id of 3 points around the plane are" << std::endl;
//	std::vector<int> ids = this->plane_pts_idx[tri_id];
//	std::cout << ids[0] << ", " << ids[1] << ", " << ids[2] << std::endl;
//	std::cout << std::endl;

    //std::cout << "The 3d coordinate of three points in the intersecting plane are:" << std::endl;
    //std::vector<double> pts1, pts2, pts3;
    //pts1.push_back(this->cloud->points[ids[0]].x);
    //pts1.push_back(this->cloud->points[ids[0]].y);
    //pts1.push_back(this->cloud->points[ids[0]].z);
    //pts2.push_back(this->cloud->points[ids[1]].x);
    //pts2.push_back(this->cloud->points[ids[1]].y);
    //pts2.push_back(this->cloud->points[ids[1]].z);
    //pts3.push_back(this->cloud->points[ids[2]].x);
    //pts3.push_back(this->cloud->points[ids[2]].y);
    //pts3.push_back(this->cloud->points[ids[2]].z);
    //print_vec(pts1);
    //print_vec(pts2);
    //print_vec(pts3);
    //std::cout << std::endl;

    //std::cout << "t is:" << t << std::endl;
    //std::cout << std::endl;

    //print_vec(cpts);
    //std::cout << std::endl;

    pts_on_mesh* ptr_mesh;
    if (tri_id == -1 || closest >= projection_err_threshold) {
        conf = 0;

        if (closest >= projection_err_threshold) {
            fprintf(stderr, "no projected triangles because of big projection error: %.2f!\n", closest);
        }
        else
            fprintf(stderr, "no projected triangles!\n");

        ptr_mesh = new pts_on_mesh(-1, -1, 0.0, 0.0, 0.0, conf);
        return ptr_mesh;
    }

    //ptr_mesh = new pts_on_mesh(this->plane_pts_idx[tri_id][0], cpts[0], cpts[1], cpts[2], conf);
    pts_3d_conf pts_3d(cpts[0], cpts[1], cpts[2], conf);
    //std::cout << "final 3d points is" << std::endl;
    //pts_3d.print();
    pts_on_mesh* pts_mesh = find_closest_pts_on_mesh(pts_3d, tri_id);
    pts_mesh->print();

    return pts_mesh;
}

// note that we add heuristic to this function, the furtherest point (within the error range) should be the right choice
pts_on_mesh* MyMesh::get_pts_on_mesh_heuristic(cv::Point3d C_src, std::vector<double>& ray, double conf, cv::Point3d pts_3d_ref) {
    std::cout << "finding the closest point on the mesh given a 3d point." << std::endl;
    clock_t old_time = clock();
    double final_dist = 1000000;
    double max_t = -1;
    std::vector<double> pts_3d_ref_vec = cv2vec_pts3d(pts_3d_ref);
    std::vector<double> cpts;
    int tri_id = -1;
    int planes = this->planes.size();
    std::vector<double> C = cv2vec_pts3d(C_src);

    // go though all planes
    float time_accu1 = 0;
    float time_accu2 = 0;
    for (int plane_index = 0; plane_index < planes; plane_index++) {
        // plane is plane[0] * x + plane[1] * y + plane[2] * z + plane[3] = 0
        // expected 3d point is [C[0] + t * ray[0], C[1] + t * ray[1], C[2] + t * ray[2]], t is the parameter we need to find
        std::vector<double>& plane = this->planes[plane_index];
        double n = 0, d = 0;
        for (int l = 0; l < 3; l++) {
            n += C[l] * plane[l];
            d += ray[l] * plane[l];
        }
        n += plane[3];
        double t = -n / d;


        clock_t new_time = clock();
        time_accu1 += float(new_time - old_time);
        old_time = new_time;

//        std::cout << "n is " << n << std::endl;
//        std::cout << "d is " << d << std::endl;

        if (std::abs(d) < EPS_SMALL) {
//            std::cout << "Skipped current plane for unknown reason 1 ...\n" << std::endl;
            continue;
        }

        // t should be larger than 0, otherwise the 3d point is in the wrong direction oppose to the ray
        if (t < 0) {
//			std::cout << "Skipped current plane because the direction is the opposite  ...\n" << std::endl;
            continue;
        }

        std::vector<double> pts_tmp;
        for (int i = 0; i < 3; i++) {
            pts_tmp.push_back(t * ray[i] + C[i]);	// the 3d point
        }

        // even though t is larger than 0, is further than a 3d point found before
        double dist = compute_distance(pts_tmp, pts_3d_ref_vec);                   // compute the distance between the found point and reference 3d point
        if (dist >= projection_err_threshold) {
//			std::cout << "Skipped current plane because the it is further than the 3d point found before..." << std::endl;
            continue;
        }

//        std::vector<double> pts_append = pts_tmp;
//        pts_append.push_back(1.0);

//		std::cout << "checking the point on triangle...\n" << std::endl;
//		ASSERT_WITH_MSG(std::abs(inner(pts_append, plane)) < EPS_MYSELF, "Point found is not even on the extended triangle plane. The offset is " + std::to_string(std::abs(inner(pts_append, plane))));		// check the point with parameter t is inside the current plane

        std::vector<int> vec_id = this->plane_pts_idx[plane_index];
        std::vector<double> tri_a, tri_b, tri_c;
        tri_a.push_back(this->cloud->points[vec_id[0]].x);
        tri_a.push_back(this->cloud->points[vec_id[0]].y);
        tri_a.push_back(this->cloud->points[vec_id[0]].z);
        tri_b.push_back(this->cloud->points[vec_id[1]].x);
        tri_b.push_back(this->cloud->points[vec_id[1]].y);
        tri_b.push_back(this->cloud->points[vec_id[1]].z);
        tri_c.push_back(this->cloud->points[vec_id[2]].x);
        tri_c.push_back(this->cloud->points[vec_id[2]].y);
        tri_c.push_back(this->cloud->points[vec_id[2]].z);

        // check if the point is inside the triangle
//		std::cout << "checking the triangle...\n" << std::endl;
        if (t > max_t) {		// find the exact right plane which intersect with the ray and the intersection point is inside the plane.
            if (point_triangle_test_3d(pts_tmp, tri_a, tri_b, tri_c)) {
                std::cout << "t is " << t <<  ", previous biggest t is " << max_t << ", current point found is " << pts_tmp[0] << ", " << pts_tmp[1] << ", " << pts_tmp[2] << ", distance is " << dist << std::endl;

                max_t = t;
                final_dist = dist;
                std::cout << "current biggest t is " << max_t << std::endl;
                tri_id = plane_index;
                cpts = pts_tmp;
            }
        }

        new_time = clock();
        time_accu2 += float(new_time - old_time);
        old_time = new_time;
    }

    pts_on_mesh* ptr_mesh;
    if (tri_id == -1 || final_dist >= projection_err_threshold) {
        conf = 0;

        if (final_dist >= projection_err_threshold) {
            fprintf(stderr, "no projected triangles because of big projection error: %.2f!\n", final_dist);
        }
        else
            fprintf(stderr, "no projected triangles!\n");

        ptr_mesh = new pts_on_mesh(-1, -1, 0.0, 0.0, 0.0, conf);
        return ptr_mesh;
    }

    //ptr_mesh = new pts_on_mesh(this->plane_pts_idx[tri_id][0], cpts[0], cpts[1], cpts[2], conf);
    pts_3d_conf pts_3d(cpts[0], cpts[1], cpts[2], conf);
    //std::cout << "final 3d points is" << std::endl;
    //pts_3d.print();
    pts_on_mesh* pts_mesh = find_closest_pts_on_mesh(pts_3d, tri_id);
    pts_mesh->print();

    return pts_mesh;
}


//pts_on_mesh* MyMesh::find_closest_pts_on_mesh(pts_3d_conf& pts_3d) {
//	//std::cout << "matching 3d point found with points on the mesh." << std::endl;
//	ASSERT_WITH_MSG(pts_3d.conf >= 0 && pts_3d.conf <= 1, "The confidence should be in the range of [0, 1].");
//	int closest_id = -1;
//	double closest_distance = -1;
//	double dist_tmp;
//	for (int i = 0; i < this->cloud->points.size(); i++) {
//		dist_tmp = sqrt((this->cloud->points[i].x - pts_3d.x) * (this->cloud->points[i].x - pts_3d.x) + (this->cloud->points[i].y - pts_3d.y) * (this->cloud->points[i].y - pts_3d.y) + (this->cloud->points[i].z - pts_3d.z) * (this->cloud->points[i].z - pts_3d.z));
//		if (closest_distance == -1 || dist_tmp < closest_distance) {
//			closest_distance = dist_tmp;
//			closest_id = i;
//		}
//	}
//	return new pts_on_mesh(closest_id, this->cloud->points[closest_id].x, this->cloud->points[closest_id].y, this->cloud->points[closest_id].z, pts_3d.conf);
//}

pts_on_mesh* MyMesh::find_closest_pts_on_mesh(pts_3d_conf& pts_3d, int plane_id) {
	//std::cout << "matching 3d point found with points on the mesh." << std::endl;
//	ASSERT_WITH_MSG(pts_3d.conf >= 0 && pts_3d.conf <= 1, "The confidence should be in the range of [0, 1].");
	int number_vertices = this->plane_pts_idx[plane_id].size();
//	std::cout << "number vertices: " << number_vertices << std::endl;
	int vertice_id_tmp;
	int closest_id = -1;
	double closest_distance = -1;
	double dist_tmp;
	for (int i = 0; i < number_vertices; i++) {
		vertice_id_tmp = this->plane_pts_idx[plane_id][i];
		//std::cout << vertice_id_tmp << std::endl;
		dist_tmp = sqrt((this->cloud->points[vertice_id_tmp].x - pts_3d.x) * (this->cloud->points[vertice_id_tmp].x - pts_3d.x) + (this->cloud->points[vertice_id_tmp].y - pts_3d.y) * (this->cloud->points[vertice_id_tmp].y - pts_3d.y) + (this->cloud->points[vertice_id_tmp].z - pts_3d.z) * (this->cloud->points[vertice_id_tmp].z - pts_3d.z));
		//std::cout << dist_tmp << std::endl;
		if (closest_distance == -1 || dist_tmp < closest_distance) {
			closest_distance = dist_tmp;
			closest_id = this->plane_pts_idx[plane_id][i];
		}
	}
	//std::cout << closest_id << std::endl;
	return new pts_on_mesh(closest_id, plane_id, this->cloud->points[closest_id].x, this->cloud->points[closest_id].y, this->cloud->points[closest_id].z, pts_3d.conf);
}