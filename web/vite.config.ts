/// <reference types="vitest" />
import { defineConfig } from "vite";
import preact from "@preact/preset-vite";
import tailwindcss from "@tailwindcss/vite";
import { viteSingleFile } from "vite-plugin-singlefile";

export default defineConfig({
  plugins: [preact(), tailwindcss(), viteSingleFile()],
  build: {
    target: "es2020",
    minify: "terser",
    cssMinify: true,
  },
  server: {
    proxy: {
      "/api": {
        target: process.env.VITE_DEVICE_IP
          ? `http://${process.env.VITE_DEVICE_IP}`
          : "http://192.168.4.1",
        changeOrigin: true,
      },
    },
  },
  test: {
    environment: "jsdom",
    globals: true,
    setupFiles: "src/test-setup.ts",
    include: ["src/**/*.test.{ts,tsx}"],
  },
});
