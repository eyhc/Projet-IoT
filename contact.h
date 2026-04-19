#ifndef CONTACT_H
#define CONTACT_H

#include "chat_data.h"

// Afficher la liste des contacts
void list_contacts(struct sync_chat_data *chat_data);

/* ========================================================================== */

// Liste seulement les contacts favoris
void list_favorite_contacts(struct sync_chat_data *chat_data);

// Ajoute un contact à la liste des favoris
// Returns :
// - 0 : succès (y compris si déjà favori)
// - 1 : il n'y a plus de place dans les contacts
int add_favorite_contact(struct sync_chat_data *chat_data,
                         const char *contact_name);

// Retire à un contact sont caractère de favori
// Returns :
// - 0 : succès
// - 1 : contact non trouvé
// - 2 : contact trouvé mais n'était pas favori
int remove_favorite_contact(struct sync_chat_data *chat_data,
                            const char *contact_name);

#endif /* CONTACT_H */
