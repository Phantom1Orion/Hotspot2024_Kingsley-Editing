#include "include/traj.h"
void get_densityfields(fields *fi, particles *pt, par *par)
{
   unsigned int n0 = n_partd; // number of particles ci[0];
   // unsigned int n = n_partd * 2;              // both electron and ion
   unsigned int n4 = n_partd * sizeof(float); // number of particles * sizeof(float)
   // unsigned int n8 = n * sizeof(float);       // number of particles * sizeof(float)
   unsigned int nc = n_cells * ncoeff * 3; // trilin constatnts have 8 coefficients 3 components
   unsigned int n_cellsi = n_cells * sizeof(int);
   unsigned int n_cellsf = n_cells * sizeof(float);
   static bool fastIO;
   static bool first = true;
   static int ncalc_e = 0, ncalc_i = 0;

   // cout << "command q" << endl; //  create queue to which we will push commands for the device.
   // cl::CommandQueue queue(context_g, default_device_g);
   cl::CommandQueue queue = commandQueue_g;
   cl::Kernel kernel_density = cl::Kernel(program_g, "density"); // select the kernel program to run
   cl::Kernel kernel_df = cl::Kernel(program_g, "df");           // select the kernel program to run
                                                                 // write input arrays to the device
   cl::Kernel kernel_dtotal = cl::Kernel(program_g, "dtotal");
   if (first)
      if (fastIO)
      { // is mapping required? // Yes we might need to map because OpenCL does not guarantee that the data will be shared, alternatively use SVM
        // auto * mapped_pt->buff_x0_e = (float *)queue.enqueueMapBuffer(pt->buff_x0_e, CL_TRUE, CL_MAP_WRITE, 0, sizeof(float) * n); queue.enqueueUnmapMemObject(pt->buff_x0_e, mapped_pt->buff_x0_e[0]);
      }
      else
      {
         queue.enqueueWriteBuffer(pt->buff_x0_e[0], CL_TRUE, 0, n4, pt->pos0x[0]);
         queue.enqueueWriteBuffer(pt->buff_y0_e[0], CL_TRUE, 0, n4, pt->pos0y[0]);
         queue.enqueueWriteBuffer(pt->buff_z0_e[0], CL_TRUE, 0, n4, pt->pos0z[0]);
         queue.enqueueWriteBuffer(pt->buff_x1_e[0], CL_TRUE, 0, n4, pt->pos1x[0]);
         queue.enqueueWriteBuffer(pt->buff_y1_e[0], CL_TRUE, 0, n4, pt->pos1y[0]);
         queue.enqueueWriteBuffer(pt->buff_z1_e[0], CL_TRUE, 0, n4, pt->pos1z[0]);

         queue.enqueueWriteBuffer(pt->buff_q_e[0], CL_TRUE, 0, n4, pt->q[0]);
         //  ions next

         queue.enqueueWriteBuffer(pt->buff_x0_i[0], CL_TRUE, 0, n4, pt->pos0x[1]);
         queue.enqueueWriteBuffer(pt->buff_y0_i[0], CL_TRUE, 0, n4, pt->pos0y[1]);
         queue.enqueueWriteBuffer(pt->buff_z0_i[0], CL_TRUE, 0, n4, pt->pos0z[1]);
         queue.enqueueWriteBuffer(pt->buff_x1_i[0], CL_TRUE, 0, n4, pt->pos1x[1]);
         queue.enqueueWriteBuffer(pt->buff_y1_i[0], CL_TRUE, 0, n4, pt->pos1y[1]);
         queue.enqueueWriteBuffer(pt->buff_z1_i[0], CL_TRUE, 0, n4, pt->pos1z[1]);

         queue.enqueueWriteBuffer(pt->buff_q_i[0], CL_TRUE, 0, n4, pt->q[1]);
      }

   queue.enqueueFillBuffer(fi->buff_npi[0], 0, 0, n_cellsi);
   queue.enqueueFillBuffer(fi->buff_cji[0], 0, 0, n_cellsi * 3);

   //  set arguments to be fed into the kernel program
   // cout << "kernel arguments for electron" << endl;

   kernel_density.setArg(0, pt->buff_x0_e[0]);          // x0
   kernel_density.setArg(1, pt->buff_y0_e[0]);          // y0
   kernel_density.setArg(2, pt->buff_z0_e[0]);          // z0
   kernel_density.setArg(3, pt->buff_x1_e[0]);          // x1
   kernel_density.setArg(4, pt->buff_y1_e[0]);          // y1
   kernel_density.setArg(5, pt->buff_z1_e[0]);          // z1
   kernel_density.setArg(6, fi->buff_npi[0]);           // npt
   kernel_density.setArg(7, fi->buff_cji[0]);           // current
   kernel_density.setArg(8, pt->buff_q_e[0]);           // q
   kernel_density.setArg(9, sizeof(float), &par->a0_f); // scale factor
                                                        // kernel_density.setArg(14, sizeof(int), n_cells);          // ncells
                                                        // cout << "run kernel for electron" << endl;

   // run the kernel
   queue.enqueueNDRangeKernel(kernel_density, cl::NullRange, cl::NDRange(n0), cl::NullRange);
   // cout << "run kernel for electron done" << endl;
   queue.finish();

   kernel_df.setArg(0, fi->buff_np_e[0]);          // np ion
   kernel_df.setArg(1, fi->buff_npi[0]);           // np ion temp integer
   kernel_df.setArg(2, fi->buff_currentj_e[0]);    // current
   kernel_df.setArg(3, fi->buff_cji[0]);           // current
   kernel_df.setArg(4, sizeof(float), &par->a0_f); // scale factor

   queue.enqueueNDRangeKernel(kernel_df, cl::NullRange, cl::NDRange(n_cells), cl::NullRange);
   queue.finish();
   // cout << "read electron density" << endl;

   queue.enqueueFillBuffer(fi->buff_npi[0], 0, 0, n_cellsi);
   queue.enqueueFillBuffer(fi->buff_cji[0], 0, 0, n_cellsi * 3);
   //  set arguments to be fed into the kernel program
   kernel_density.setArg(0, pt->buff_x0_i[0]);          // x0
   kernel_density.setArg(1, pt->buff_y0_i[0]);          // y0
   kernel_density.setArg(2, pt->buff_z0_i[0]);          // z0
   kernel_density.setArg(3, pt->buff_x1_i[0]);          // x1
   kernel_density.setArg(4, pt->buff_y1_i[0]);          // y1
   kernel_density.setArg(5, pt->buff_z1_i[0]);          // z1
   kernel_density.setArg(6, fi->buff_npi[0]);           // npt
   kernel_density.setArg(7, fi->buff_cji[0]);           // current
   kernel_density.setArg(8, pt->buff_q_i[0]);           // q
   kernel_density.setArg(9, sizeof(float), &par->a0_f); // scale factor
                                                        // kernel_density.setArg(14, sizeof(int), &n_cells);          // ncells
                                                        // cout << "run kernel for ions" << endl;
   //  run the kernel
   queue.enqueueNDRangeKernel(kernel_density, cl::NullRange, cl::NDRange(n0), cl::NullRange);
   queue.finish();                                 // wait for the end of the kernel program
   kernel_df.setArg(0, fi->buff_np_i[0]);          // np ion
   kernel_df.setArg(1, fi->buff_npi[0]);           // np ion temp integer
   kernel_df.setArg(2, fi->buff_currentj_i[0]);    // current
   kernel_df.setArg(3, fi->buff_cji[0]);           // current
   kernel_df.setArg(4, sizeof(float), &par->a0_f); // scale factor
   queue.enqueueNDRangeKernel(kernel_df, cl::NullRange, cl::NDRange(n_cells), cl::NullRange);
   queue.finish();

   kernel_dtotal.setArg(0, fi->buff_np_e[0]);       // np ion
   kernel_dtotal.setArg(1, fi->buff_np_i[0]);       // np ion
   kernel_dtotal.setArg(2, fi->buff_currentj_e[0]); // current
   kernel_dtotal.setArg(3, fi->buff_currentj_i[0]); // current
   kernel_dtotal.setArg(4, fi->buff_npt[0]);        // total particles density
   kernel_dtotal.setArg(5, fi->buff_jc[0]);         // total current density
   kernel_dtotal.setArg(6, sizeof(size_t), &n_cells);
   queue.enqueueNDRangeKernel(kernel_dtotal, cl::NullRange, cl::NDRange(n_cells / 16), cl::NullRange);
   queue.finish();
   // read result arrays from the device to main memory
   if (first)
      if (fastIO)
      { // is mapping required?
        // mapped_pt->buff_x0_e = (float *)queue.enqueueMapBuffer(pt->buff_x0_e[0], CL_TRUE, CL_MAP_READ, 0, sizeof(float) * n); queue.enqueueUnmapMemObject(pt->buff_x0_e[0], mapped_pt->buff_x0_e[0]);
      }
      else
      {
         queue.enqueueReadBuffer(pt->buff_q_e[0], CL_TRUE, 0, n4, pt->q[0]);
         queue.enqueueReadBuffer(pt->buff_q_i[0], CL_TRUE, 0, n4, pt->q[1]);

         queue.enqueueReadBuffer(fi->buff_np_e[0], CL_TRUE, 0, n_cellsf, fi->np[0]);
         queue.enqueueReadBuffer(fi->buff_np_i[0], CL_TRUE, 0, n_cellsf, fi->np[1]);

         queue.enqueueReadBuffer(fi->buff_currentj_e[0], CL_TRUE, 0, n_cellsf * 3, fi->currentj[0]);
         queue.enqueueReadBuffer(fi->buff_currentj_i[0], CL_TRUE, 0, n_cellsf * 3, fi->currentj[1]);

         queue.enqueueReadBuffer(fi->buff_npt[0], CL_TRUE, 0, n_cellsf, fi->npt);
         queue.enqueueReadBuffer(fi->buff_jc[0], CL_TRUE, 0, n_cellsf * 3, fi->jc);
      }

   first = false;
}