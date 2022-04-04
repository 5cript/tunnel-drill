import './main.css';
import React, {useState} from 'react';
import Sidebar from '../../components/drawer';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import AppBar from '@mui/material/AppBar';
import CssBaseline from '@mui/material/CssBaseline';
import Toolbar from '@mui/material/Toolbar';
import InboxIcon from '@mui/icons-material/MoveToInbox';
import MailIcon from '@mui/icons-material/Mail';
import Dashboard from '../dashboard/dashboard';
import {
  Routes,
  Route,
  useNavigate,
  Link
} from "react-router-dom";
import Backend from '../../backend_api/backend';

const SidebarWidth = 240;
const backend = new Backend();

const Main = () => {
  const navigate = useNavigate();

  const navigateTo = (section, item) => {
    navigate('/frontend/' + section + '/' + item);
  }

  const getDashboard = () => {
    return <Dashboard 
      backend={backend}
    />
  }

  return (
    <div className="Main">
      <Box sx={{ display: 'flex' }}>
        <CssBaseline />
        <AppBar position="fixed" sx={{ zIndex: (theme) => theme.zIndex.drawer + 1 }}>
          <Toolbar>
            <Typography variant="h6" noWrap component="div">
              {"Tunnel Bore Dashboard"}
            </Typography>
          </Toolbar>
        </AppBar>
        <Sidebar 
          width={SidebarWidth}
          sections={[
            {
              items: [
                {
                  title: 'Start',
                  icon: () => {
                    return <MailIcon />
                  }
                },
                {
                  title: 'TBD'
                }
              ]
            },
            {
              items: [
                {
                  title: 'TBD'
                }
              ]
            }
          ]}
          onNavigate={navigateTo}
        />
        <Box component="main" sx={{ flexGrow: 1, p: 3 }}>
          <Toolbar />
          <Routes>
            <Route path="*" element={getDashboard()} />
            <Route path="/frontend/0/Start" element={getDashboard()} />
            <Route path="/frontend/0/TBD" element={<div>{"B"}</div>} />
            <Route path="/frontend/1/TBD" element={<div>{"C"}</div>} />
          </Routes>
        </Box>
      </Box>
    </div>
  );
}

export default Main;
