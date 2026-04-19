#ifndef GROUP_H
#define GROUP_H

#include "chat_data.h"

// Rejoindre un groupe de discussion
// Returns :
// - 0 : succès
// - 1 : le groupe existe déjà
// - 2 : la liste des groupes est pleine
int join_group(struct sync_chat_data *chat_data, const char *group_name);

// Quitter un groupe de discussion
// Returns :
// - 0 : succès
// - 1 : le groupe n'existe pas
int leave_group(struct sync_chat_data *chat_data, const char *group_name);

// Afficher la liste des groupes de discussion auxquels je suis inscrit
void list_groups(struct sync_chat_data *chat_data);

#endif /* GROUP_H */
