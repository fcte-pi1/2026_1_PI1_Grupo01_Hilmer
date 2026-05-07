import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

const backendTarget =
  process.env.VITE_API_PROXY_TARGET || "http://localhost:3001";

export default defineConfig({
  plugins: [react()],
  server: {
    host: "127.0.0.1",
    port: 5173,
    proxy: {
      "/api": {
        target: backendTarget,
        changeOrigin: true
      }
    }
  }
});
