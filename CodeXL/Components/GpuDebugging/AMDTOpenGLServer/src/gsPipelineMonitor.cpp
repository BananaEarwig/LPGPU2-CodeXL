//==================================================================================
// Copyright (c) 2016 , Advanced Micro Devices, Inc.  All rights reserved.
//
/// \author AMD Developer Tools Team
/// \file gsPipelineMonitor.cpp
///
//==================================================================================
#include <src/gsPipelineMonitor.h>
#include <AMDTBaseTools/Include/gtAssert.h>
#include <AMDTBaseTools/Include/gtString.h>
#include <AMDTServerUtilities/Include/suAllocatedObjectsMonitor.h>

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::gsPipelineMonitor
// Description: Default CTOR
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
gsPipelineMonitor::gsPipelineMonitor() : m_glPipelines(), mCurrentlyBoundPipelineName(0)
{
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::~gsPipelineMonitor
// Description: DTOR
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
gsPipelineMonitor::~gsPipelineMonitor()
{
    // Delete all objects.
    for (size_t i = 0; i < m_glPipelines.size(); i++)
    {
        delete m_glPipelines[i];
        m_glPipelines[i] = NULL;
    }
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::GenProgramPipelines
// Description: Creates new program pipeline objects
// Arguments:   GLuint pipelinesCount - amount of pipeline objects to be created
//              const GLuint* pPipelinesArr - array of previously unused pipeline
//              names (generated by the OpenGL runtime)
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
void gsPipelineMonitor::GenProgramPipelines(GLuint pipelinesCount, const GLuint* pPipelinesArr)
{
    gtVector<apAllocatedObject*> pipelinesForAllocationMonitor;

    for (size_t i = 0; i < pipelinesCount; i++)
    {
        // Create a stateless Program Pipeline object.
        apGLPipeline* pCreatedPipeline = new apGLPipeline(pPipelinesArr[i]);

        // Save the generated pipeline.
        m_glPipelines.push_back(pCreatedPipeline);
        pipelinesForAllocationMonitor.push_back(pCreatedPipeline);
    }

    // Register the pipeline objects in the allocated objects monitor.
    suAllocatedObjectsMonitor::instance().registerAllocatedObjects(pipelinesForAllocationMonitor);
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::DeleteProgramPipelines
// Description: Deletes the program pipelines requested
// Arguments:   GLuint pipelinesCount - amount of pipeline objects to be deleted
//              const GLuint* pPipelinesArr - array with the names of the program
//              pipelines to be deleted
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
void gsPipelineMonitor::DeleteProgramPipelines(GLuint pipelinesCount, const GLuint* pPipelinesArr)
{
    for (size_t i = 0; i < pipelinesCount; i++)
    {
        // Get the name of the relevant pipeline object.
        const GLuint pipelineName = pPipelinesArr[i];

        GT_IF_WITH_ASSERT(pipelineName > 0)
        {
            bool isFound = false;
            int itemIndex = -1;

            // Find the relevant pipeline object in our pipelines container.
            int pipelineCount = (int)m_glPipelines.size();

            for (int j = 0; j < pipelineCount; j++)
            {
                const apGLPipeline* pCurrItem = m_glPipelines[j];
                isFound = (pCurrItem != NULL && pCurrItem->pipelineName() == pipelineName);

                if (isFound)
                {
                    // If we found the relevant item, store its index and break the loop.
                    itemIndex = j;
                    break;
                }
            }

            GT_IF_WITH_ASSERT(itemIndex > -1 && itemIndex < static_cast<int>(m_glPipelines.size()))
            {
                // Delete the relevant pipeline object.
                delete m_glPipelines[itemIndex];
                m_glPipelines[itemIndex] = NULL;

                // Remove the deleted item from the container.
                m_glPipelines.removeItem(itemIndex);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::BindProgramPipeline
// Description: Sets the requested program pipeline as bound
// Arguments:   GLuint pipelineName - the name of the pipeline to be set as bound
// Note:        if pipelineName is zero all pipelines become unbound, otherwise
//              all pipelines except for pipelineName become unbound.
// Return value - bool (success/failure)
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
bool gsPipelineMonitor::BindProgramPipeline(GLuint pipelineName)
{
    bool ret = false;

    // If the pipeline name is zero, it means that no pipeline
    // should be bound. If pipeline neither exists (has been generated by
    // glGenProgramPipeline() nor equals zero, then it is an OGL runtime error.
    if (pipelineName == 0 || isPipelineExists(pipelineName))
    {
        mCurrentlyBoundPipelineName = pipelineName;

        for (size_t i = 0; i < m_glPipelines.size(); ++i)
        {
            if (m_glPipelines[i] != NULL)
            {
                // Make the pipeline name the only bound pipeline.
                m_glPipelines[i]->setIsPipelineBound(m_glPipelines[i]->pipelineName() == pipelineName);
            }
        }

        ret = true;
    }

    return ret;
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::GetCurrentlyBoundPipeline
// Description: Returns the name of the currently bound program pipeline
// Return value - GLuint (the name of the currently bound PP)
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
GLuint gsPipelineMonitor::GetCurrentlyBoundPipeline() const
{
    return mCurrentlyBoundPipelineName;
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::ActiveShaderProgram
// Description: Sets the given program as the active program for the given pipeline
// Arguments:   GLuint pipeline - the name of the pipeline
//              GLuint program  - the name of the program
// Return value - bool (success/failure)
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
bool gsPipelineMonitor::ActiveShaderProgram(GLuint pipeline, GLuint program)
{
    bool ret = false;

    for (size_t i = 0; !ret && i < m_glPipelines.size(); i++)
    {
        GT_IF_WITH_ASSERT(m_glPipelines[i] != NULL)
        {
            // Find the relevant pipeline.
            if (pipeline == m_glPipelines[i]->pipelineName())
            {
                // Update the relevant pipeline.
                m_glPipelines[i]->setActiveProgram(program);
                ret = true;
            }
        }
    }

    return ret;
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::isPipelineExists
// Description: Returns true if and only if a pipeline with the given name exists
// Arguments:   GLuint pipelineName - the name of the pipeline
// Return value - bool (exists or not)
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
bool gsPipelineMonitor::isPipelineExists(GLuint pipelineName) const
{
    // Note that zero pipeline may always be valid.
    bool isPipelineExists = (pipelineName == 0);

    for (size_t i = 0; !isPipelineExists && i < m_glPipelines.size(); i++)
    {
        GT_IF_WITH_ASSERT(m_glPipelines[i] != NULL)
        {
            isPipelineExists = (pipelineName == (m_glPipelines[i]->pipelineName()));
        }
    }

    return isPipelineExists;
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::UseProgramStages
// Description: Sets the program as the active program for the pipeline for all
//              pipeline stages specified
// Arguments:   GLuint pipeline - the name of the pipeline
//              GLbitfield stages - the pipeline stages
//              GLuint program - the name of the program
// Return value - bool (success/failure)
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
bool gsPipelineMonitor::UseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program)
{
    bool ret = false;

    for (size_t i = 0; !ret && i < m_glPipelines.size(); i++)
    {
        GT_IF_WITH_ASSERT(m_glPipelines[i] != NULL)
        {
            // Find the relevant pipeline.
            if (pipeline == m_glPipelines[i]->pipelineName())
            {
                // Update the relevant pipeline.
                m_glPipelines[i]->useProgramStages(stages, program);
                ret = true;
            }
        }
    }

    return ret;
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::GetAmountOfPipelineObjects
// Description: Returns the number of program pipeline objects that are being
//              monitored by this object
// Return value - size_t (the amount of PPOs)
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
size_t gsPipelineMonitor::GetAmountOfPipelineObjects() const
{
    return m_glPipelines.size();
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::GetPipelineDetails
// Description: Returns a pointer to the requested pipeline object's details
// Return value const apGLPipeline*
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
const apGLPipeline* gsPipelineMonitor::GetPipelineDetails(GLuint pipelineName) const
{
    const apGLPipeline* retVal = nullptr;

    for (const apGLPipeline* pCurrentPipeLine : m_glPipelines)
    {
        GT_IF_WITH_ASSERT(nullptr != pCurrentPipeLine)
        {
            if (pCurrentPipeLine->pipelineName() == pipelineName)
            {
                // We found the relevant pipeline.
                retVal = pCurrentPipeLine;
            }
        }
    }

    return retVal;
}

// ---------------------------------------------------------------------------
// Name:        gsPipelineMonitor::GetPipelineNameByIndex
// Description: Returns the name of the program pipeline object whose index in
//              the monitor's internal storage is the given index
// Arguments:   size_t pipelineIndex - the given index
//              GLuint& pipelineName - an output parameter to hold the name of the
//              program requested pipeline object (note that the value in this
//              variable should be treated as valid only if the function succeeds
//              (returns true).
// Return value - bool (success/failure)
// Author:      Amit Ben-Moshe
// Date:        25/6/2014
// ---------------------------------------------------------------------------
bool gsPipelineMonitor::GetPipelineNameByIndex(size_t pipelineIndex, GLuint& pipelineNameBuffer) const
{
    bool ret = false;

    GT_IF_WITH_ASSERT(pipelineIndex < m_glPipelines.size())
    {
        GT_IF_WITH_ASSERT(m_glPipelines[pipelineIndex] != NULL)
        {
            pipelineNameBuffer = m_glPipelines[pipelineIndex]->pipelineName();
            ret = true;
        }
    }

    return ret;
}