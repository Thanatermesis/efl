#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Get the class of a given part.
 *
 * @param[in] part The part object.
 * @return The class of the part, or @c NULL on error.
 */
EOLIAN_API const Eolian_Class *
eolian_part_class_get(const Eolian_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   return part->klass;
}

/**
 * @brief Get the documentation of a given part.
 *
 * @param[in] part The part object.
 * @return The documentation of the part, or @c NULL on error or if no
 * documentation exists.
 */
EOLIAN_API const Eolian_Documentation *
eolian_part_documentation_get(const Eolian_Part *part)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(part, NULL);
   return part->doc;
}
