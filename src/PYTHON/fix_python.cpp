/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "fix_python.h"
#include "atom.h"
#include "force.h"
#include "update.h"
#include "respa.h"
#include "error.h"
#include "python.h"

using namespace LAMMPS_NS;
using namespace FixConst;

// Wrap API changes between Python 2 and 3 using macros
#if PY_MAJOR_VERSION == 2
#define PY_VOID_POINTER(X) PyCObject_FromVoidPtr((void *) X, NULL)
#elif PY_MAJOR_VERSION == 3
#define PY_VOID_POINTER(X) PyCapsule_New((void *) X, NULL, NULL)
#endif

/* ---------------------------------------------------------------------- */

FixPython::FixPython(LAMMPS *lmp, int narg, char **arg) :
  Fix(lmp, narg, arg)
{
  if (narg != 5) error->all(FLERR,"Illegal fix python command");

  // ensure Python interpreter is initialized
  python->init();

  if (strcmp(arg[3],"post_force") == 0) {
    selected_callback = POST_FORCE;
  } else if (strcmp(arg[3],"end_of_step") == 0) {
    selected_callback = END_OF_STEP;
  }

  // get Python function
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject * pyMain = PyImport_AddModule("__main__");

  if (!pyMain) {
    PyGILState_Release(gstate);
    error->all(FLERR,"Could not initialize embedded Python");
  }

  char * fname = arg[4];
  pFunc = PyObject_GetAttrString(pyMain, fname);

  if (!pFunc) {
    PyGILState_Release(gstate);
    error->all(FLERR,"Could not find Python function");
  }

  PyGILState_Release(gstate);
}

/* ---------------------------------------------------------------------- */

int FixPython::setmask()
{
  return selected_callback;
}

/* ---------------------------------------------------------------------- */

void FixPython::end_of_step()
{
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject * ptr = PY_VOID_POINTER(lmp);
  PyObject * arglist = Py_BuildValue("(O)", ptr);

  PyObject * result = PyEval_CallObject(pFunc, arglist);
  Py_DECREF(arglist);

  PyGILState_Release(gstate);
}

/* ---------------------------------------------------------------------- */

void FixPython::post_force(int vflag)
{
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject * ptr = PY_VOID_POINTER(lmp);
  PyObject * arglist = Py_BuildValue("(Oi)", ptr, vflag);

  PyObject * result = PyEval_CallObject(pFunc, arglist);
  Py_DECREF(arglist);

  PyGILState_Release(gstate);
}
