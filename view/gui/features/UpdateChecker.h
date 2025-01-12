#pragma once

/**
 * A module responsible for online update checking.
 */
namespace UpdateChecker
{
    /**
     * Checks for updates.
     * @param manual Whether the update check was initiated via a deliberate user interaction.
     */
    void check(bool manual);
};
