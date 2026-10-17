/* tdnl.79: drive the REAL demo resource through the Menu Manager. */
#include <stdio.h>
#include "ref_demo_menu.h"
#include "test_assert.h"
TEST_HARNESS();
int main(void)
{
    MenuItem it[2]={flair_demo_item("New",'N'),flair_demo_item("Open",'O')};
    MenuInfo menu={256,"File",it,2,0};
    MenuBar hello={&menu,1,0};
    for(int i=0;i<flair_ten_notes_bar.n_menus;i++)
        for(int j=0;j<flair_ten_notes_bar.menus[i].n_items;j++)
            CHECK(!flair_ten_notes_bar.menus[i].items[j].enabled,"DEMO_GRAY NOTES has no enabled dead action");
    CHECK(!MenuKey(&hello,'N') && !MenuKey(&hello,'O'),"DEMO_GRAY HELLO keys cannot select dead actions");
    CHECK(!MenuKey(&flair_ten_notes_bar,'Q'),"DEMO_GRAY NOTES Quit cannot select a dead action");
    return TEST_SUMMARY("test-finder-demo");
}
