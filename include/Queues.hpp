#ifndef QUEUES
#define QUEUES

/*-----------------------------------------------------------------------------*/
/** 
 * @file   Queues.hpp
 * @brief  Header for Queues struct 
 * 
 * The Queues struct is used for the definition of RTOS queues used for data
 * logging through the codebase
 * 
 * @author Lilia Turbeville
 * @date   June 16, 2026
*//*---------------------------------------------------------------------------*/

#include "arduino_freertos.h"
#include "semphr.h"

/**
 * @brief Namespace used for general RTOS values, types, and methods
 */
namespace RTOS {
  /**
   * @brief Struct used to hold queues used through the codebase
   */
  struct Queues {
    static QueueHandle_t logQueue;   ///< Queue for Serial Monitor output
    static QueueHandle_t errorQueue; ///< Queue for error output

    /**
     * @brief Method used to write messages to the log queue for Serial Monitor
     * 
     * @param msg Message to write as a char pointer
     */
    static void logWrite(const char * msg) {
      xQueueSend(logQueue, (void *)&msg, pdMS_TO_TICKS(10));
    }


    /**
     * @brief Method used to write error messages to error logging queue 
     * 
     * @param err Error message to write as a char pointer 
     */
    static void errorWrite(const char * err) {
      xQueueSend(errorQueue, (void *)&err, pdMS_TO_TICKS(10));
    }
  };

  QueueHandle_t Queues::logQueue = xQueueCreate(16, sizeof(const char*));
  QueueHandle_t Queues::errorQueue = xQueueCreate(8, sizeof(const char*));
}

#endif // QUEUES