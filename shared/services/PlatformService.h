#pragma once

/*
 *	Interface for special platform-related functionality.
 *
 *	Must be implemented in the view layer.
 */

namespace PlatformService
{
    /**
     * Gets the free() function from the C runtime of the specified module.
     * \param module The handle to a platform-specific module type to get the function from.
     * \return The free function in the module.
     */
    void(* get_free_function_in_module(void* module))(void*);
}
