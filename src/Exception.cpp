#include "Exception.h"
#include "Objects/UserObject.h"
#include "Objects/StringObject.h"

Variable RuntimeException::ToVMException() const
{
	UserDefinedType* type = nullptr;
    if (GetManager().GetType(type, TypeName)) {
		auto obj = GetManager().Make(type->Type);

		obj.as<UserObject>()->SetField("message", String::GetAllocator()->Make(what()));
		return obj;
    }
	gScriptLogger() << "Exception type not found: " << TypeName.toString() << "\n";
	return Variable();
}
