import { Navigate, Route, Routes } from 'react-router-dom';
import { DefaultLayout } from './layouts/DefaultLayout';
import { Dashboard } from './pages/Dashboard';
import { History } from './pages/History';
import { NotFound } from './pages/NotFound';
import { Home } from './pages/Home';
import { NewAttempt } from './pages/NewAttempt';

export default function App() {
  return (
    <Routes>

      {/* Páginas sem layout (tela cheia) */}
      <Route path="/" element={<Home />} />
      <Route path="/new-attempt" element={<NewAttempt />} />

      {/* Páginas com DefaultLayout */}
      <Route element={<DefaultLayout />}>
        <Route path="/dashboard" element={<Dashboard />} />
        <Route path="/history" element={<History />} />
      </Route>
      <Route path="*" element={<NotFound />} />
    </Routes>
  );
}
