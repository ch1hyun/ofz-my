"""An LLM agent to analyze and provide insight of a fuzz target's low coverage.
Use it as a usual module locally, or as script in cloud builds.
"""
from agent.base_agent import BaseAgent


class CoverageAnalyzer(BaseAgent):
  pass
