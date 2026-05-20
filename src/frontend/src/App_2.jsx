import { BrowserRouter, Routes, Route, Link } from 'react-router-dom'
import Placeholder from './pages/placeholder'

function App_2() {
    return (
        <BrowserRouter>
            <Link to="/placeholder">Open Placeholder</Link>
            <Routes>
                <Route path="/placeholder" element={<Placeholder />} />
            </Routes>
        </BrowserRouter>
    )
}

export default App_2;