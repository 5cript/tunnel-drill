import * as React from 'react';
import Box from '@mui/material/Box';
import Drawer from '@mui/material/Drawer';
import CssBaseline from '@mui/material/CssBaseline';
import Toolbar from '@mui/material/Toolbar';
import List from '@mui/material/List';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import ListItem from '@mui/material/ListItem';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';

interface SidebarItem {
  title: string;
  icon?: () => any
}

interface SidebarSection {
  items: Array<SidebarItem>
}

interface SidebarConfig {
  width: number;
  sections: Array<SidebarSection>,
  onNavigate: (sectionIndex: number, itemName: string) => void
}

export default function Sidebar({
  width,
  sections,
  onNavigate
}: SidebarConfig) {
  return (
    <Box sx={{ display: 'flex' }}>
      <CssBaseline />
      <Drawer
        variant="permanent"
        sx={{
          width: width,
          flexShrink: 0,
          [`& .MuiDrawer-paper`]: { width: width, boxSizing: 'border-box' },
        }}
      >
        <Toolbar />
        <Box sx={{ overflow: 'auto' }}>
          {
            sections.map((section, sectionIndex) => {
              return <div key={sectionIndex}>
                <List>
                  {
                    section.items.map((item, index) => {
                      return <ListItem button key={item.title + sectionIndex} onClick={() => {
                        onNavigate(sectionIndex, item.title);
                      }}>
                        <ListItemIcon>
                          {item.icon ? item.icon() : <></>}
                        </ListItemIcon>
                        <ListItemText primary={item.title} />
                      </ListItem>
                    })
                  }
                </List>
                <Divider key={'d' + sectionIndex}/>
              </div>
            })
          }
        </Box>
      </Drawer>
    </Box>
  );
}
