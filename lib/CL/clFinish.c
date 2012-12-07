/* OpenCL runtime library: clFinish()

   Copyright (c) 2011 Erik Schnetter
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include "pocl_cl.h"
#include "utlist.h"

CL_API_ENTRY cl_int CL_API_CALL
POclFinish(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
  int i;
  _cl_command_node *node;
  
  if (command_queue->properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
    POCL_ABORT_UNIMPLEMENTED();
  
  /* Step through the list of commands in-order */  
  LL_FOREACH (command_queue->root, node)
    {
      cl_event *event = &node->event;
      switch (node->type)
        {
        case CL_COMMAND_READ_BUFFER:
          POCL_UPDATE_EVENT_SUBMITTED;
          POCL_UPDATE_EVENT_RUNNING;
          command_queue->device->read
            (node->command.read.data, 
             node->command.read.host_ptr, 
             node->command.read.device_ptr, 
             node->command.read.cb); 
          POCL_UPDATE_EVENT_COMPLETE;
          POclReleaseMemObject (node->command.read.buffer);
          break;
        case CL_COMMAND_WRITE_BUFFER:
          POCL_UPDATE_EVENT_SUBMITTED;
          POCL_UPDATE_EVENT_RUNNING;
          command_queue->device->write
            (node->command.write.data, 
             node->command.write.host_ptr, 
             node->command.write.device_ptr, 
             node->command.write.cb);
          POCL_UPDATE_EVENT_COMPLETE;
          POclReleaseMemObject (node->command.write.buffer);
          break;
        case CL_COMMAND_COPY_BUFFER:
          POCL_UPDATE_EVENT_SUBMITTED;
          POCL_UPDATE_EVENT_RUNNING;
          command_queue->device->copy
            (node->command.copy.data, 
             node->command.copy.src_ptr, 
             node->command.copy.dst_ptr,
             node->command.copy.cb);
          POCL_UPDATE_EVENT_COMPLETE;
          POclReleaseMemObject (node->command.copy.src_buffer);
          POclReleaseMemObject (node->command.copy.dst_buffer);
          break;
        case CL_COMMAND_MAP_BUFFER: 
          {
            POCL_WARN_UNTESTED();
            POCL_UPDATE_EVENT_SUBMITTED;
            POCL_UPDATE_EVENT_RUNNING;            
            pocl_map_mem_cmd (command_queue->device, node->command.map.buffer, 
                              node->command.map.mapping);
            POCL_UPDATE_EVENT_COMPLETE;
            break;
          }
        case CL_COMMAND_NDRANGE_KERNEL:
          assert (*event == node->event);
          POCL_UPDATE_EVENT_SUBMITTED;
          POCL_UPDATE_EVENT_RUNNING;
          command_queue->device->run(node->command.run.data, node);
          POCL_UPDATE_EVENT_COMPLETE;
          for (i = 0; i < node->command.run.arg_buffer_count; ++i)
            {
              cl_mem buf = node->command.run.arg_buffers[i];
              if (buf == NULL) continue;
              //printf ("### releasing arg %d - the buffer %x of kernel %s\n", i, buf,  node->command.run.kernel->function_name);
              POclReleaseMemObject (buf);
            }
          free (node->command.run.arg_buffers);
          free (node->command.run.tmp_dir);
          POclReleaseKernel(node->command.run.kernel);
          break;  
        case CL_COMMAND_MARKER:
          POCL_UPDATE_EVENT_COMPLETE;
          break;
        default:
          POCL_ABORT_UNIMPLEMENTED();
          break;
        }
    } 
  
  // free the queue contents
  node = command_queue->root;
  command_queue->root = NULL;
  while (node)
    {
      _cl_command_node *tmp;
      tmp = node->next;
      free (node);
      node = tmp;
    }  

  return CL_SUCCESS;
}
POsym(clFinish)
