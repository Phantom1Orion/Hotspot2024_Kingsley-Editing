_DEPS = traj.h traj_physics.h
_OBJ = utils.o TS3.o tnp.o generate.o generaterandp.o  save.o cl_code.o changedtdx.o calcEBV_vkFFT.o calcU.o  get_densityfields.o utils_VkFFT.o

IDIR = include
dir_guard=@mkdir -p $(@D)
#https://stackoverflow.com/questions/14492436/g-optimization-beyond-o3-ofast
CC=g++
#ucrt64
CFLAGS= -pg -no-pie -I$(IDIR) -I /ucrt64/include/vtk -L /ucrt64/lib/vtk -march=native -malign-double -std=c++2b  -fopenmp -fopenmp-simd 


CFLAGS+= -O3 -ftree-parallelize-loops=8 
CFLAGS+= -mavx -mavx2 -mfma -ffast-math -ftree-vectorize -fno-omit-frame-pointer #-finline-functions
LIBS= -lm  -lOpenCL  -lgomp #-lgsl# -lfftw3f -lfftw3f_omp 
LIBS+= -lvtkCommonCore.dll  -lvtksys.dll -lvtkIOXML.dll -lvtkCommonDataModel.dll -lvtkIOCore.dll


AFLAGS= -flto=$(CPUS) -funroll-loops -fno-signed-zeros -fno-trapping-math -D_GLIBCXX_PARALLEL -fgcse-sm -fgcse-las 
#-Wl,--stack,4294967296

CFLAGS += $(AFLAGS)
CPUS ?= $(shell (nproc --all || sysctl -n hw.ncpu) 2>/dev/null || echo 1)
MAKEFLAGS += --jobs=$(CPUS)

ODIR=obj
DODIR=obj_debug
LDIR=lib

DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
OBJvk = $(patsubst %,$(ODIR)/%,$(_OBJvk))
DOBJ = $(patsubst %,$(DODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(dir_guard)
	$(CC) -c -o $@ $< $(CFLAGS)

$(DODIR)/%.o: %.cpp $(DEPS)
	$(dir_guard)
	$(CC) -g -c -o $@ $< $(CFLAGSd)

TS3: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)


debug: $(DOBJ)
	$(CC) -v -o TS3$@ $^ $(CFLAGSd) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(DODIR)/*.o *~ core $(INCDIR)/*~ TS3.exe TS3debug.exe *.vti *.vtp *.vts TS3