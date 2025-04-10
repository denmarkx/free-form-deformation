#ifndef LINESEGS_EXT_H
#define LINESEGS_EXT_H

#include "nodePath.h"
#include "lineSegs.h"

namespace LINESEGS_EXT {
    NodePath process_lines(LineSegs& lines, NodePath& lineNP);
    void update_lines(LineSegs& line, NodePath& lineNP);
}

#endif