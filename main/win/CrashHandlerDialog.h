#pragma once

#include <Windows.h>

/// <summary>
/// A WinAPI dialog for displaying crash-related actions and information
/// </summary>
class CrashHandlerDialog
{
public:

	/// <summary>
	/// Choices specifying how the program shall proceed after the crash
	/// </summary>
	enum class Choices
	{
		/// <summary>
		/// Attempt to recover and proceed
		/// </summary>
		Ignore,
		/// <summary>
		/// Exit the program
		/// </summary>
		Exit
	};

	/// <summary>
	/// Types specifying what choices the dialog provides
	/// </summary>
	enum class Types
	{
		/// <summary>
		/// The crash can be ignored
		/// </summary>
		Ignorable = 1,
	};

	CrashHandlerDialog(Types type, const char* caption) {
		Type = type;
		Caption = caption;
	}

	~CrashHandlerDialog() = default;

	/// <summary>
	///		Shows the dialog
	/// </summary>
	/// <returns>
	///		The choice selected by the user
	/// </returns>
	Choices Show();

	/// <summary>
	/// <para>
	///		The current <see cref="CrashHandlerDialogChoices"/>
	/// </para>
	/// <b>
	///		This field is static, as WinAPI window procedures can't be declared as members of C++ classes
	/// </b>
	/// </summary>
	static inline Choices CrashHandlerDialogChoice;

	/// <summary>
	/// <para>
	///		The global dialog type
	/// </para>
	/// <b>
	///		This field is static, as WinAPI window procedures can't be declared as members of C++ classes
	/// </b>
	/// </summary>
	static inline Types Type;

	/// <summary>
	/// <para>
	///		The global dialog caption
	/// </para>
	/// <b>
	///		This field is static, as WinAPI window procedures can't be declared as members of C++ classes
	/// </b>
	/// </summary>
	static inline const char* Caption;
};