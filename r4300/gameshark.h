#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace Gameshark
{
	/**
	 * \brief Represents a gameshark script
	 * \warning Thread-unsafe
	 */
	class Script
	{
	public:
		/**
		 * \brief Sets the resumed state
		 * \param resumed Whether the script is allowed to execute code
		 */
		void set_resumed(bool resumed)
		{
			m_resumed = resumed;
		}

		/**
		 * \brief Executes the script code
		 */
		void execute();

		/**
		 * \brief Compiles a script from a string
		 * \param code The script code
		 * \return The compiled script, or none if the script is invalid
		 */
		static std::optional<std::shared_ptr<Script>> compile(const std::string& code);

	private:
		// Whether the script code gets executed
		bool m_resumed = true;

		// Pair 1st element tells us whether instruction is a conditional. That's required for special treatment of buggy kaze blj anywhere code
		std::vector<std::tuple<bool, std::function<bool()>>> m_instructions;
	};

	/**
	 * \brief Adds a script to the gameshark execution list
	 * \param script The script
	 */
	void add_script(std::shared_ptr<Script> script);

	/**
	 * \brief Executes the gameshark code
	 */
	void execute();
}
