/*
 * main.cc
 *
 *  Created on: Jul 26, 2016
 *      Author: kristjan
 */

#include <cstdlib>
#include <iostream>

#include <deal.II/base/timer.h>

#include "laplace.h"
#include "physical_quantities.h"
#include "currents_and_heating.h"


int main() {

	dealii::Timer timer;

	fch::PhysicalQuantities pq;

	std::string path1 = "../res/physical_quantities/";
	std::string path2 = "heating/res/physical_quantities/";

	if (pq.load_emission_data(path1+"gtf_grid_1000x1000.dat") &&
		pq.load_nottingham_data(path1+"nottingham_grid_300x300.dat") &&
		pq.load_resistivity_data(path1+"cu_res.dat"))
		std::cout << "Found pq data in " + path1 << std::endl;
	else if (pq.load_emission_data(path2+"gtf_grid_1000x1000.dat") &&
			 pq.load_nottingham_data(path2+"nottingham_grid_300x300.dat") &&
			 pq.load_resistivity_data(path2+"cu_res.dat"))
		std::cout << "Found pq data in " + path2 << std::endl;
	else
		return EXIT_FAILURE;

	std::cout << "    Load PhysicalQuantities: " << timer.wall_time() << " s" << std::endl; timer.restart();

	// ------------------------------------------------------------------------------------- //
	// Laplace solver //
	fch::Laplace<3> laplace_solver;
	laplace_solver.import_mesh_from_file("../res/3d_meshes/nanotip_vacuum.msh");
	laplace_solver.setup_system();
	laplace_solver.assemble_system();
	laplace_solver.solve();
	laplace_solver.output_results("output/field_sol.vtk");

	std::cout << laplace_solver << std::endl;

	std::cout << "    Solved laplace_solver: " << timer.wall_time() << " s" << std::endl; timer.restart();

	// Accessing data in vertexes
	std::vector<int> cell_indexes, vertex_indexes;
	for (int i = 0; i < 10; i++) {
		cell_indexes.push_back(i);
		vertex_indexes.push_back(0);
	}
	std::vector<double> potentials = laplace_solver.get_potential(cell_indexes, vertex_indexes);
	std::vector<dealii::Tensor<1, 3>> fields = laplace_solver.get_efield(cell_indexes, vertex_indexes);

	// ------------------------------------------------------------------------------------- //
	// Currents and heating //
	fch::CurrentsAndHeating<3> ch_solver(&pq, &laplace_solver);
	ch_solver.import_mesh_from_file("../res/3d_meshes/nanotip_copper.msh");

	double final_error = ch_solver.run_specific(10.0, 3, true, "output/sol", true);

	std::cout << "    Solved currents&heating: " << timer.wall_time() << " s" << std::endl; timer.restart();
	std::cout << "    Final temp. error: " << final_error << std::endl;

	std::cout << ch_solver << std::endl;

	// ------------------------------------------------------------------------------------- //
	// Currents and heating resume (or next iteration, which resumes from previous iteration) //
	fch::CurrentsAndHeating<3> ch_solver2(&pq, &laplace_solver, &ch_solver);
	ch_solver2.import_mesh_from_file("../res/3d_meshes/nanotip_copper.msh");

	final_error = ch_solver2.run_specific(10.0, 3, true, "output/sol_next", true);

	std::cout << "    Solved currents&heating: " << timer.wall_time() << " s" << std::endl; timer.restart();
	std::cout << "    Final temp. error: " << final_error << std::endl;

	// Access data in vertexes
	std::vector<double> temperatures = ch_solver2.get_temperature(cell_indexes, vertex_indexes);
	std::vector<dealii::Tensor<1, 3>> currents = ch_solver2.get_current(cell_indexes, vertex_indexes);

	for (unsigned int i = 0; i < temperatures.size(); i++) {
		std::cout << i << " T: " << temperatures[i] << "; C: " << currents[i] << std::endl;
	}

	// ------------------------------------------------------------------------------------- //
	// Laplace solver //
	fch::Laplace<3> laplace_solver2;
	laplace_solver2.import_mesh_from_file("../res/3d_meshes/mushroom_vacuum.msh");
	laplace_solver2.setup_system();
	laplace_solver2.assemble_system();
	laplace_solver2.solve();
	laplace_solver2.output_results("output/field_sol2.vtk");

	std::cout << "    Solved laplace_solver: " << timer.wall_time() << " s" << std::endl; timer.restart();

	// ------------------------------------------------------------------------------------- //
	// Currents and heating reuse ch_solver by taking ch_solver2 as initial condition //

	ch_solver.reinitialize(&laplace_solver2, &ch_solver2);
	ch_solver.import_mesh_from_file("../res/3d_meshes/mushroom_copper.msh");

	final_error = ch_solver.run_specific(1.0, 5, true, "output/sol_nnext", true);

	std::cout << "    Solved currents&heating: " << timer.wall_time() << " s" << std::endl; timer.restart();
	std::cout << "    Final temp. error: " << final_error << std::endl;


/* 2d case usage */
/*
	laplace::Laplace<2> field;
	field.import_mesh_from_file("../res/2d_meshes/vacuum_aligned_dense.msh", "vacuum_mesh_2d.vtk");
	field.run();

	currents_heating::CurrentsAndHeating<2> ch(pq, &field);
	ch.import_mesh_from_file("../res/2d_meshes/copper_aligned_dense.msh", "copper_mesh_2d.vtk");
	ch.run();
*/

/* 2d Mesh splitting */
/*
	MeshPreparer<2> mesh_preparer;
	Triangulation<2> mesh;
	mesh_preparer.import_mesh_from_file(&mesh, "../res/2d_meshes/both_dense.msh");
	Triangulation<2> new_mesh = mesh_preparer.remove_cells_with_id(&mesh, 20);
	mesh_preparer.mark_vacuum_boundary(&new_mesh);
	mesh_preparer.output_mesh(&new_mesh, "vacuum_aligned_dense.msh");
*/

/* FCH usage */
/*
	MeshPreparer<2> mesh_preparer_fch;
	field_currents_heating::FieldCurrentsHeating<2> fch(pq);

	Triangulation<2> *p_mesh = fch.getp_triangulation();
	mesh_preparer_fch.import_mesh_from_file(p_mesh, "../res/2d_meshes/mod.msh");
	mesh_preparer_fch.mark_top_and_bottom_boundary(p_mesh);
	mesh_preparer_fch.output_mesh(p_mesh, "mesh_2d.vtk");

	fch.run();
*/
	return EXIT_SUCCESS;
}



