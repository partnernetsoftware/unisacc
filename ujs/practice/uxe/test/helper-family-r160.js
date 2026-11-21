import { REVISION } from "../../web/game/three.module.js";

export function observeDisposal(cases) {
  if (REVISION !== "160") throw new Error("three.js revision mismatch");
  const values = cases.map(([helper, ...owners]) => {
    if (!owners.length) owners.push(helper);
    const resources = owners.flatMap(({ geometry, material }) => [geometry, material]);
    const expected = new Map();
    const events = new Map();
    for (const resource of resources) {
      if (!expected.has(resource)) {
        events.set(resource, 0);
        resource.addEventListener("dispose", () => events.set(resource, events.get(resource) + 1));
      }
      expected.set(resource, (expected.get(resource) ?? 0) + 1);
    }
    helper.dispose();
    return Number(Array.from(expected).every(([resource, count]) => events.get(resource) === count));
  });
  document.querySelector("#r160").textContent = JSON.stringify(values);
}
