import { render } from "preact";
import "./styles/app.css";
import { App } from "./app";

const root = document.getElementById("app");
if (root) render(<App />, root);
