
#include "DependencyGraph.h"

NAMESPACE_GNXENGINE_BEGIN

class FrameGraph
{
public:
    DependencyGraph& getGraph()
    {
        return mGraph;
    }
    DependencyGraph& mGraph;
};

NAMESPACE_GNXENGINE_END
